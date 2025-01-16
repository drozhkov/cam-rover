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

#ifndef __ROVER__HTTP__H
#define __ROVER__HTTP__H


#include "esp_http_server.h"


typedef void ( *t_rover_http_handler_post_wlan_config )( const char * ssid, const char * password );


void rover_http_set_handler_post_wlan_config( t_rover_http_handler_post_wlan_config handler );

esp_err_t rover_http_root_handler( httpd_req_t * req );
esp_err_t rover_http_404_error_handler( httpd_req_t * req, httpd_err_code_t err );


#endif
