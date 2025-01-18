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

#include <sys/param.h>

#include "lwip/inet.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "esp_netif.h"
#include "esp_wifi.h"

#include "nvs_flash.h"

#include "dns_server.h"
#include "esp_http_server.h"

#include "types.h"
#include "globals.h"
#include "helpers.h"
#include "config.h"
#include "wifi.h"
#include "http.h"
#include "discovery.h"
#include "camera.h"
#include "comm_udp.h"
#include "drive.h"


static const char * roverLogTAG = "rover";

static char roverHostname[] = "camrover-\0MMAACC";

static t_rover_discovery roverDiscovery;
static t_rover_comm_udp roverCommControl = { 0 };
static t_rover_comm_udp roverCommStreaming = { 0 };
static t_rover_camera roverCamera = { 0 };
static t_rover_drive roverDrive = { 0 };


static void rover_comm_handler_move_stop( void )
{
	rover_drive_change_speed( &roverDrive, -roverDrive.motor1.speed, -roverDrive.motor2.speed );
}


static t_rover_motors_speed rover_comm_handler_move_speed( int32_t inc )
{
	return rover_drive_change_speed( &roverDrive, inc, inc );
}


static t_rover_motors_speed rover_comm_handler_move_set( int32_t speedL, int32_t speedR )
{
	return rover_drive_change_speed( &roverDrive, speedL - roverDrive.motor1.speed, speedR - roverDrive.motor2.speed );
}


t_rover_motors_speed rover_comm_handler_move_turn( int32_t incL, int32_t incR )
{
	return rover_drive_change_speed( &roverDrive, incL, incR );
}


void rover_comm_handler_move_deadzone( uint32_t v )
{
	roverDrive.deadzone = v;
}


void rover_comm_handler_camera_flash( uint8_t duty )
{
	rover_camera_set_flash_duty( &roverCamera, duty );
}


static void rover_camera_handler_frame( t_rover_camera_frame * frame )
{
	rover_comm_udp_send( &roverCommStreaming, frame->ptr, frame->len );
}


static void rover_http_handler_post_wlan_config( const char * ssid, const char * password )
{
	t_rover_config config;
	config.wlan.ssid = ssid;
	config.wlan.password = password;

	rover_save_config( &config );
	ESP_LOGI(
		roverLogTAG, "WLAN configuration saved: SSID = %s, password = %s", config.wlan.ssid, config.wlan.password );

	esp_restart();
}


static httpd_handle_t rover_start_webserver( void )
{
	static const httpd_uri_t root = { .uri = "/", .method = HTTP_ANY, .handler = rover_http_root_handler };

	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_open_sockets = 8;
	config.lru_purge_enable = true;

	// Start the httpd server
	ESP_LOGI( roverLogTAG, "Starting server on port: '%d'", config.server_port );

	if ( httpd_start( &server, &config ) == ESP_OK ) {
		// Set URI handlers
		ESP_LOGI( roverLogTAG, "Registering URI handlers" );
		httpd_register_uri_handler( server, &root );
		httpd_register_err_handler( server, HTTPD_404_NOT_FOUND, rover_http_404_error_handler );
	}

	return server;
}


void app_main( void )
{
	esp_log_level_set( "httpd_uri", ESP_LOG_ERROR );
	esp_log_level_set( "httpd_txrx", ESP_LOG_ERROR );
	esp_log_level_set( "httpd_parse", ESP_LOG_ERROR );

	// Initialize networking stack
	ESP_ERROR_CHECK( esp_netif_init() );

	// Create default event loop needed by the  main app
	ESP_ERROR_CHECK( esp_event_loop_create_default() );

	// Initialize NVS needed by Wi-Fi
	ESP_ERROR_CHECK( nvs_flash_init() );

	rover_drive_init( &roverDrive );

	uint8_t mac[6];
	esp_read_mac( mac, ESP_MAC_WIFI_SOFTAP );
	char macString[( sizeof mac ) * 2 + 1];
	rover_to_hex( macString, mac + 2, 4 );
	strcat( roverHostname, macString );

	t_rover_config roverConfig;

	if ( rover_load_config( &roverConfig ) ) {
		ESP_LOGI( roverLogTAG,
			"WLAN configuration loaded: SSID = %s, password = %s",
			roverConfig.wlan.ssid,
			roverConfig.wlan.password );

		esp_netif_set_hostname( esp_netif_create_default_wifi_sta(), roverHostname );

		if ( !rover_wifi_init_sta( roverConfig.wlan.ssid, roverConfig.wlan.password ) ) {
			rover_reset_config();
			esp_restart();
		}
	}
	else {
		esp_netif_set_hostname( esp_netif_create_default_wifi_ap(), roverHostname );
		rover_wifi_init_softap( macString );

		// Start the DNS server that will redirect all queries to the softAP IP
		dns_server_config_t config =
			DNS_SERVER_CONFIG_SINGLE( "*" /* all A queries */, "WIFI_AP_DEF" /* softAP netif ID */ );

		start_dns_server( &config );
	}

	rover_http_set_handler_post_wlan_config( rover_http_handler_post_wlan_config );

	roverCamera.frameHandler = rover_camera_handler_frame;
	rover_camera_start( &roverCamera );

	rover_start_webserver();

	// control
	roverCommControl.handlers.move.speed = rover_comm_handler_move_speed;
	roverCommControl.handlers.move.stop = rover_comm_handler_move_stop;
	roverCommControl.handlers.move.turn = rover_comm_handler_move_turn;
	roverCommControl.handlers.move.set = rover_comm_handler_move_set;
	roverCommControl.handlers.move.deadzone = rover_comm_handler_move_deadzone;
	roverCommControl.handlers.camera.flash = rover_comm_handler_camera_flash;
	roverCommControl.portNo = 101;
	roverDiscovery.controlPortNo = rover_comm_udp_start( &roverCommControl );

	// streaming
	roverCommStreaming.portNo = 102;
	roverDiscovery.cameraStreamPortNo = rover_comm_udp_start( &roverCommStreaming );

	rover_discovery_start( &roverDiscovery );
}
