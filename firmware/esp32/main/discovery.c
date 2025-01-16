/*
 *	Copyright (c) 2025 Denis Rozhkov <denis@rozhkoff.com>
 *	This file is part of cam-rover.
 *
 *	cam-rover is free software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	cam-rover is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *	Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along with
 *	cam-rover. If not, see <https://www.gnu.org/licenses/>.
 */

// SPDX-FileCopyrightText: 2025 Denis Rozhkov <denis@rozhkoff.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdint.h>

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "esp_log.h"

#include "globals.h"
#include "helpers.h"
#include "discovery.h"


#define ROVER_DISCOVERY_IPV4_ADDR "239.255.255.250"
#define ROVER_DISCOVERY_TTL 8
#define ROVER_DISCOVERY_UDP_PORT 3703


struct {
	const char * address;
	uint8_t ttl;
	uint16_t portNo;
} roverConfigDiscovery = {
	.address = ROVER_DISCOVERY_IPV4_ADDR, .ttl = ROVER_DISCOVERY_TTL, .portNo = ROVER_DISCOVERY_UDP_PORT
};

static const char * roverLogTAG = "rover.discovery";


static int rover_discovery_add_ipv4_multicast_group( int socketFd )
{
	struct ip_mreq mreq = { 0 };
	struct in_addr addr = { 0 };
	int err = 0;

	// Configure source interface
	mreq.imr_interface.s_addr = IPADDR_ANY;
	// Configure multicast address to listen to
	err = inet_aton( roverConfigDiscovery.address, &mreq.imr_multiaddr.s_addr );

	if ( err != 1 ) {
		ESP_LOGE( roverLogTAG, "Configured IPV4 multicast address '%s' is invalid.", roverConfigDiscovery.address );
		// Errors in the return value have to be negative
		err = -1;
		goto _l_exit;
	}

	ESP_LOGI( roverLogTAG, "Configured IPV4 Multicast address %s", inet_ntoa( mreq.imr_multiaddr.s_addr ) );

	// Assign the IPv4 multicast source interface, via its IP
	// (only necessary if this socket is IPV4 only)
	err = setsockopt( socketFd, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof( struct in_addr ) );

	if ( err < 0 ) {
		ESP_LOGE( roverLogTAG, "Failed to set IP_MULTICAST_IF. Error %d", errno );
		goto _l_exit;
	}

	err = setsockopt( socketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof( struct ip_mreq ) );

	if ( err < 0 ) {
		ESP_LOGE( roverLogTAG, "Failed to set IP_ADD_MEMBERSHIP. Error %d", errno );
		goto _l_exit;
	}

_l_exit:
	return err;
}


static int rover_discovery_create_multicast_ipv4_socket( void )
{
	struct sockaddr_in addr = { 0 };
	int socketFd = -1;
	int err = 0;

	socketFd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );

	if ( socketFd < 0 ) {
		ESP_LOGE( roverLogTAG, "Failed to create socket. Error %d", errno );
		return -1;
	}

	// Bind the socket to any address
	addr.sin_family = PF_INET;
	addr.sin_port = htons( roverConfigDiscovery.portNo );
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	err = bind( socketFd, (struct sockaddr *)&addr, sizeof( struct sockaddr_in ) );

	if ( err < 0 ) {
		ESP_LOGE( roverLogTAG, "Failed to bind socket. Error %d", errno );
		goto _l_close;
	}

	// Assign multicast TTL (set separately from normal interface TTL)
	err = setsockopt( socketFd, IPPROTO_IP, IP_MULTICAST_TTL, &roverConfigDiscovery.ttl, sizeof( uint8_t ) );

	if ( err < 0 ) {
		ESP_LOGE( roverLogTAG, "Failed to set IP_MULTICAST_TTL. Error %d", errno );
		goto _l_close;
	}

	// this is also a listening socket, so add it to the multicast
	// group for listening...
	err = rover_discovery_add_ipv4_multicast_group( socketFd );

	if ( err < 0 ) {
		goto _l_close;
	}

	// All set, socket is configured for sending and receiving
	return socketFd;

_l_close:
	close( socketFd );
	return -1;
}


static void rover_discovery_task( void * pvParameters )
{
	t_rover_discovery * discovery = (t_rover_discovery *)pvParameters;
	// char probeMatch[] = "CAM-ROVER:PROBE_MATCH:\0PORTC:PORTS";
	char probeMatch[64];

	snprintf( probeMatch,
		sizeof probeMatch,
		"CAM-ROVER:PROBE_MATCH:%" PRIu16 ":%" PRIu16,
		discovery->controlPortNo,
		discovery->cameraStreamPortNo );

	size_t probeMatchLen = strlen( probeMatch );

	while ( true ) {
		int socketFd = rover_discovery_create_multicast_ipv4_socket();

		if ( socketFd < 0 ) {
			ESP_LOGE( roverLogTAG, "Failed to create IPv4 multicast socket" );
		}

		if ( socketFd < 0 ) {
			// Nothing to do!
			vTaskDelay( 5 / portTICK_PERIOD_MS );
			continue;
		}

		// Loop waiting for UDP received, and sending UDP packets if we don't
		// see any.
		int err = 1;

		while ( err > 0 ) {
			struct timeval tv = {
				.tv_sec = 2,
				.tv_usec = 0,
			};

			fd_set rfds;
			FD_ZERO( &rfds );
			FD_SET( socketFd, &rfds );

			int s = select( socketFd + 1, &rfds, NULL, NULL, &tv );

			if ( s < 0 ) {
				ESP_LOGE( roverLogTAG, "Select failed: errno %d", errno );
				err = -1;
				break;
			}

			if ( s > 0 && FD_ISSET( socketFd, &rfds ) ) {
				// Incoming datagram received
				char recvbuf[48];
				char raddr_name[32] = { 0 };

				struct sockaddr_storage raddr; // Large enough for both IPv4 or IPv6
				socklen_t socklen = sizeof( raddr );
				int len = recvfrom( socketFd, recvbuf, sizeof( recvbuf ) - 1, 0, (struct sockaddr *)&raddr, &socklen );

				if ( len < 0 ) {
					ESP_LOGE( roverLogTAG, "multicast recvfrom failed: errno %d", errno );
					err = -1;
					break;
				}

				// Get the sender's address as a string
				if ( PF_INET == raddr.ss_family ) {
					inet_ntoa_r( ( (struct sockaddr_in *)&raddr )->sin_addr, raddr_name, sizeof( raddr_name ) - 1 );
				}

				ESP_LOGI( roverLogTAG, "received %d bytes from %s:", len, raddr_name );

				recvbuf[len] = 0; // Null-terminate whatever we received and treat like a string...
				ESP_LOGI( roverLogTAG, "%s", recvbuf );

				if ( ROVER_IS_STRING_EQ( "CAM-ROVER:PROBE", recvbuf ) ) {
					sendto( socketFd, probeMatch, probeMatchLen, 0, (struct sockaddr *)&raddr, socklen );
				}
			}
		}

		ESP_LOGE( roverLogTAG, "Shutting down socket and restarting..." );
		shutdown( socketFd, 0 );
		close( socketFd );
	}
}


void rover_discovery_start( t_rover_discovery * discovery )
{
	xTaskCreate( &rover_discovery_task, "rover_discovery_task", 4096, discovery, 5, NULL );
}
