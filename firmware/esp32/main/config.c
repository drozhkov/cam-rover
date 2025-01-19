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

#include <stdbool.h>

#include "nvs_flash.h"

#include "config.h"


#define ROVER_CONFIG_VERSION 5

static const char roverNvsNamespace[] = "rover";
static const char roverNvsKeyWlanSsid[] = "wlan.ssid";
static const char roverNvsKeyWlanPassword[] = "wlan.password";
static const char roverNvsKeyConfigVersion[] = "config.version";


static const char * rover_config_get_string( nvs_handle_t h, const char * key )
{
	size_t len;

	if ( nvs_get_str( h, key, NULL, &len ) != ESP_OK ) {
		return NULL;
	}

	char * r = malloc( len );
	nvs_get_str( h, key, r, &len );

	return r;
}


bool rover_load_config( t_rover_config * out )
{
	nvs_handle_t h;

	if ( nvs_open( roverNvsNamespace, NVS_READONLY, &h ) != ESP_OK ) {
		return false;
	}

	bool r = false;
	uint8_t configVersion;

	if ( nvs_get_u8( h, roverNvsKeyConfigVersion, &configVersion ) != ESP_OK ||
		configVersion != ROVER_CONFIG_VERSION ) {

		goto l_close;
	}

	out->wlan.ssid = rover_config_get_string( h, roverNvsKeyWlanSsid );

	if ( NULL == out->wlan.ssid ) {
		goto l_close;
	}

	out->wlan.password = rover_config_get_string( h, roverNvsKeyWlanPassword );

	r = true;

l_close:
	nvs_close( h );

	return r;
}


void rover_save_config( const t_rover_config * config )
{
	nvs_handle_t h;

	if ( nvs_open( roverNvsNamespace, NVS_READWRITE, &h ) != ESP_OK ) {
		return;
	}

	nvs_set_u8( h, roverNvsKeyConfigVersion, 0 );

	nvs_set_str( h, roverNvsKeyWlanSsid, config->wlan.ssid );
	nvs_set_str( h, roverNvsKeyWlanPassword, config->wlan.password );

	nvs_set_u8( h, roverNvsKeyConfigVersion, ROVER_CONFIG_VERSION );

	nvs_close( h );
}


void rover_reset_config( void )
{
	nvs_handle_t h;

	if ( nvs_open( roverNvsNamespace, NVS_READWRITE, &h ) != ESP_OK ) {
		return;
	}

	nvs_set_u8( h, roverNvsKeyConfigVersion, 0 );
	nvs_close( h );
}
