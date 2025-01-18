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

#include <string.h>

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "esp_log.h"

#include "helpers.h"
#include "comm.h"
#include "comm_udp.h"


static const char * roverLogTAG = "rover.comm-udp";


static int rover_comm_udp_create_socket( uint16_t * portNo )
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
	// addr.sin_port = htons( 0 );
	addr.sin_port = htons( *portNo );
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	err = bind( socketFd, (struct sockaddr *)&addr, sizeof( struct sockaddr_in ) );

	if ( err < 0 ) {
		ESP_LOGE( roverLogTAG, "Failed to bind socket. Error %d", errno );
		goto _l_close;
	}

	socklen_t addrLen = sizeof addr;
	err = getsockname( socketFd, (struct sockaddr *)&addr, &addrLen );

	if ( err < 0 ) {
		ESP_LOGE( roverLogTAG, "Failed to get socket address. Error %d", errno );
		goto _l_close;
	}

	*portNo = ntohs( addr.sin_port );
	ESP_LOGI( roverLogTAG, "UDP comm port: %d", (int)*portNo );

	// All set, socket is configured for sending and receiving
	return socketFd;

_l_close:
	close( socketFd );
	return -1;
}


static void rover_comm_udp_task( void * pvParameters )
{
	uint8_t buffer[16];
	t_rover_comm_udp * commUdp = (t_rover_comm_udp *)pvParameters;

	while ( true ) {
		int socketFd = commUdp->socketFd;
		ESP_LOGI( roverLogTAG, "listening on socket: %d", socketFd );

		int err = 1;

		struct sockaddr_storage raddr = { 0 }; // Large enough for both IPv4 or IPv6
		socklen_t socklen = sizeof raddr;
		struct timeval tv = {
			.tv_sec = 2,
			.tv_usec = 0,
		};

		fd_set rfds;
		t_rover_motors_speed motorsSpeed = { 0 };

		while ( err > 0 ) {
			uint32_t messageId = 0;

			FD_ZERO( &rfds );
			FD_SET( socketFd, &rfds );

			int s = select( socketFd + 1, &rfds, NULL, NULL, &tv );

			if ( s < 0 ) {
				ESP_LOGE( roverLogTAG, "Select failed: errno %d", errno );
				// err = -1;
				continue;
			}

			bool shouldSendAck = true;

			if ( s > 0 && FD_ISSET( socketFd, &rfds ) ) {
				int len = recvfrom( socketFd, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *)&raddr, &socklen );

				if ( len < 0 ) {
					ESP_LOGE( roverLogTAG, "recvfrom failed: errno %d", errno );
					continue;
				}

				clock_gettime( CLOCK_MONOTONIC, &commUdp->lastReceiveTs );

				// ESP_LOGI( roverLogTAG, "recvfrom: len %d", len );
				if ( memcmp( &commUdp->clientAddress, &raddr, sizeof commUdp->clientAddress ) != 0 ) {
					ESP_LOGI( roverLogTAG, "client address updated" );
					commUdp->clientAddress = *( (struct sockaddr *)&raddr );
				}

				size_t messageLen = ROVER_COMM_MESSAGE_PAYLOAD_LEN( buffer );

				if ( ( messageLen + 1 ) != len ) {
					continue;
				}

				messageId = ROVER_COMM_MESSAGE_ID( buffer );
				t_rover_comm_command cmd = ROVER_COMM_MESSAGE_COMMAND( buffer );

				if ( ROVER_COMM_COMMAND_MOVE_SPEED_UP == cmd ) {
					ROVER_CALL_FUNC(
						commUdp->handlers.move.speed, motorsSpeed, ROVER_COMM_MESSAGE_MOVE_SPEED( buffer ) );
				}
				else if ( ROVER_COMM_COMMAND_MOVE_SPEED_DOWN == cmd ) {
					ROVER_CALL_FUNC(
						commUdp->handlers.move.speed, motorsSpeed, -ROVER_COMM_MESSAGE_MOVE_SPEED( buffer ) );
				}
				else if ( ROVER_COMM_COMMAND_MOVE_TURN_LEFT == cmd ) {
					ROVER_CALL_FUNC( commUdp->handlers.move.turn,
						motorsSpeed,
						-ROVER_COMM_MESSAGE_MOVE_SPEED( buffer ),
						ROVER_COMM_MESSAGE_MOVE_SPEED( buffer ) );
				}
				else if ( ROVER_COMM_COMMAND_MOVE_TURN_RIGHT == cmd ) {
					ROVER_CALL_FUNC( commUdp->handlers.move.turn,
						motorsSpeed,
						ROVER_COMM_MESSAGE_MOVE_SPEED( buffer ),
						-ROVER_COMM_MESSAGE_MOVE_SPEED( buffer ) );
				}
				else if ( ROVER_COMM_COMMAND_MOVE_SET == cmd ) {
					ROVER_CALL_FUNC( commUdp->handlers.move.set,
						motorsSpeed,
						ROVER_COMM_MESSAGE_MOVE_SPEED_L( buffer ),
						ROVER_COMM_MESSAGE_MOVE_SPEED_R( buffer ) );
				}
				else if ( ROVER_COMM_COMMAND_MOVE_STOP == cmd ) {
					ROVER_CALL( commUdp->handlers.move.stop );
					motorsSpeed.motor1 = 0;
					motorsSpeed.motor2 = 0;
				}
				else if ( ROVER_COMM_COMMAND_MOVE_DEADZONE == cmd ) {
					ROVER_CALL( commUdp->handlers.move.deadzone, ROVER_COMM_MESSAGE_MOVE_DEADZONE( buffer ) );
				}
				else if ( ROVER_COMM_COMMAND_CAMERA_FLASH == cmd ) {
					ROVER_CALL( commUdp->handlers.camera.flash, ROVER_COMM_MESSAGE_CAMERA_FLASH_DUTY( buffer ) );
				}
				else if ( ROVER_COMM_COMMAND_ACK == cmd ) {
					shouldSendAck = false;
				}
			}

			if ( raddr.s2_len != 0 && shouldSendAck ) {
				t_rover_buffer ackMessage;
				rover_comm_message_init( &ackMessage, buffer, ROVER_COMM_COMMAND_ACK, messageId );
				rover_comm_message_serialzie_u32( &ackMessage, motorsSpeed.motor1 );
				rover_comm_message_serialzie_u32( &ackMessage, motorsSpeed.motor2 );
				rover_comm_message_update_payload_len( &ackMessage );
				rover_comm_udp_send( commUdp, ackMessage.data, ackMessage.len );
			}
		}

		// ESP_LOGE( roverLogTAG, "Shutting down socket and restarting..." );
		// shutdown( socketFd, 0 );
		// close( socketFd );
	}
}


void rover_comm_udp_send( t_rover_comm_udp * commUdp, uint8_t * data, size_t dataLen )
{
	if ( 0 == commUdp->clientAddress.sa_len ) {
		// ESP_LOGW( roverLogTAG, "no dest address" );
		return;
	}

	if ( dataLen > ( 1024 * 64 ) - 128 ) {
		ESP_LOGE( roverLogTAG, "invalid UDP packet size" );
		return;
	}

	// ESP_LOGI( roverLogTAG, "sending data..." );

	xSemaphoreTake( commUdp->sync, portMAX_DELAY );
	ssize_t sentBytes =
		sendto( commUdp->socketFd, data, dataLen, 0, &commUdp->clientAddress, sizeof commUdp->clientAddress );

	if ( sentBytes != dataLen ) {
		ESP_LOGE( roverLogTAG, "sendto failed %d", errno );
	}
	xSemaphoreGive( commUdp->sync );
}


uint16_t rover_comm_udp_start( t_rover_comm_udp * commUdp )
{
	uint16_t portNo = commUdp->portNo;
	int socketFd = rover_comm_udp_create_socket( &portNo );

	if ( portNo > 0 ) {
		commUdp->socketFd = socketFd;
		commUdp->sync = xSemaphoreCreateMutexStatic( &commUdp->syncBuffer );
		xTaskCreate( &rover_comm_udp_task, "rover_comm_udp_task", 4096, commUdp, 5, NULL );
	}

	return portNo;
}
