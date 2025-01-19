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

#include "lwip/inet.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"

#include "globals.h"
#include "wifi.h"


#define ROVER_WIFI_CONNECTED_BIT BIT0
#define ROVER_WIFI_FAIL_BIT BIT1


static EventGroupHandle_t roverWifiEventGroup;
static int roverWifiStaRetryNum = 0;

static const char * roverLogTAG = "rover.wifi";


static void rover_wifi_event_handler( void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data )
{
	if ( WIFI_EVENT_AP_STACONNECTED == event_id ) {
		wifi_event_ap_staconnected_t * event = (wifi_event_ap_staconnected_t *)event_data;
		ESP_LOGI( roverLogTAG, "station " MACSTR " join, AID=%d", MAC2STR( event->mac ), event->aid );
	}
	else if ( WIFI_EVENT_AP_STADISCONNECTED == event_id ) {
		wifi_event_ap_stadisconnected_t * event = (wifi_event_ap_stadisconnected_t *)event_data;
		ESP_LOGI( roverLogTAG,
			"station " MACSTR " leave, AID=%d, reason=%d",
			MAC2STR( event->mac ),
			event->aid,
			event->reason );
	}
	else if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START ) {
		esp_wifi_connect();
	}
	else if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED ) {
		if ( roverWifiStaRetryNum < CONFIG_ESP_MAXIMUM_RETRY ) {
			esp_wifi_connect();
			roverWifiStaRetryNum++;
			ESP_LOGI( roverLogTAG, "retry to connect to the AP" );
		}
		else {
			xEventGroupSetBits( roverWifiEventGroup, ROVER_WIFI_FAIL_BIT );
		}

		ESP_LOGI( roverLogTAG, "connect to the AP fail" );
	}
	else if ( event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP ) {
		ip_event_got_ip_t * event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI( roverLogTAG, "got ip:" IPSTR, IP2STR( &event->ip_info.ip ) );
		roverWifiStaRetryNum = 0;
		xEventGroupSetBits( roverWifiEventGroup, ROVER_WIFI_CONNECTED_BIT );
	}
}


void rover_wifi_init_softap( const char * macString, t_rover_wifi_handler_scan_completed onScanCompleted )
{
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
	ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT, ESP_EVENT_ANY_ID, &rover_wifi_event_handler, NULL ) );

	wifi_config_t wifiConfig = {
		.ap = { .max_connection = CONFIG_ESP_MAX_STA_CONN, .authmode = WIFI_AUTH_WPA_WPA2_PSK },
	};

	strcpy( (char *)wifiConfig.ap.ssid, "rover-" );
	strcat( (char *)wifiConfig.ap.ssid, macString );
	wifiConfig.ap.ssid_len = strlen( (char *)wifiConfig.ap.ssid );
	strcpy( (char *)wifiConfig.ap.password, macString );

	ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_APSTA ) );
	ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_AP, &wifiConfig ) );

	wifi_config_t wifiStaConfig = { .sta = { .threshold.authmode = WIFI_AUTH_OPEN,
										.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
										.sae_h2e_identifier = "",
										.scan_method = WIFI_ALL_CHANNEL_SCAN } };

	esp_wifi_set_config( WIFI_IF_STA, &wifiStaConfig );
	esp_netif_create_default_wifi_sta();

	ESP_ERROR_CHECK( esp_wifi_start() );

	esp_netif_ip_info_t ip_info;
	esp_netif_get_ip_info( esp_netif_get_handle_from_ifkey( "WIFI_AP_DEF" ), &ip_info );

	char ip_addr[16];
	inet_ntoa_r( ip_info.ip.addr, ip_addr, 16 );
	ESP_LOGI( roverLogTAG, "Set up softAP with IP: %s", ip_addr );

	ESP_LOGI(
		roverLogTAG, "wifi_init_softap finished. SSID:'%s' password:'%s'", wifiConfig.ap.ssid, wifiConfig.ap.password );

	esp_err_t wifiError;
	wifi_ap_record_t * apRecords = NULL;

	if ( ( wifiError = esp_wifi_scan_start( NULL, true ) ) != ESP_OK ) {
		goto _l_error;
	}

	uint16_t apNum;

	if ( ( wifiError = esp_wifi_scan_get_ap_num( &apNum ) ) != ESP_OK ) {
		goto _l_error;
	}

	{
		// wifi_ap_record_t apRecords[apNum];
		apRecords = calloc( apNum, sizeof( wifi_ap_record_t ) );
		// memset( apRecords, 0, sizeof( wifi_ap_record_t ) * apNum );
		uint16_t actualApNum = apNum;

		if ( ( wifiError = esp_wifi_scan_get_ap_records( &actualApNum, apRecords ) ) != ESP_OK ) {
			goto _l_error;
		}

		if ( onScanCompleted != NULL ) {
			char * ssidBuffer[apNum];

			for ( size_t i = 0; i < apNum; i++ ) {
				ssidBuffer[i] = (char *)apRecords[i].ssid;
			}

			t_rover_string_array ssidList = { .s = ssidBuffer, .count = apNum };
			onScanCompleted( ssidList );
		}
	}

	goto _l_free_scan_records;

_l_error:
_l_free_scan_records:
	if ( apRecords != NULL ) {
		free( apRecords );
	}
}


bool rover_wifi_init_sta( const char * ssid, const char * password )
{
	roverWifiEventGroup = xEventGroupCreate();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK( esp_event_handler_instance_register(
		WIFI_EVENT, ESP_EVENT_ANY_ID, &rover_wifi_event_handler, NULL, &instance_any_id ) );

	ESP_ERROR_CHECK( esp_event_handler_instance_register(
		IP_EVENT, IP_EVENT_STA_GOT_IP, &rover_wifi_event_handler, NULL, &instance_got_ip ) );

	wifi_config_t wifi_config = {
		.sta = {
			.threshold.authmode = WIFI_AUTH_OPEN,
			.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
			.sae_h2e_identifier = "",
		},
	};

	strcpy( (char *)wifi_config.sta.ssid, ssid );
	strcpy( (char *)wifi_config.sta.password, password );

	ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
	ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &wifi_config ) );
	ESP_ERROR_CHECK( esp_wifi_start() );

	ESP_LOGI( roverLogTAG, "wifi_init_sta finished." );

	EventBits_t bits = xEventGroupWaitBits(
		roverWifiEventGroup, ROVER_WIFI_CONNECTED_BIT | ROVER_WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY );

	bool r = false;

	if ( bits & ROVER_WIFI_CONNECTED_BIT ) {
		ESP_LOGI( roverLogTAG, "connected to ap SSID:%s password:%s", ssid, password );
		r = true;
	}
	else if ( bits & ROVER_WIFI_FAIL_BIT ) {
		ESP_LOGI( roverLogTAG, "Failed to connect to SSID:%s, password:%s", ssid, password );
	}
	else {
		ESP_LOGE( roverLogTAG, "UNEXPECTED EVENT" );
	}

	return r;
}
