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

#include "esp_log.h"

#include "globals.h"

#include "helpers.h"
#include "http.h"


extern const char roverHttpHtmlRootStart[] asm( "_binary_root_html_start" );
extern const char roverHttpHtmlRootEnd[] asm( "_binary_root_html_end" );

t_rover_http_handler_post_wlan_config roverHttpHandlerPostWlanConfig = NULL;

static const char * roverLogTAG = "rover.http";


void rover_http_set_handler_post_wlan_config( t_rover_http_handler_post_wlan_config handler )
{
	roverHttpHandlerPostWlanConfig = handler;
}


esp_err_t rover_http_root_handler( httpd_req_t * req )
{
	const ssize_t root_len = roverHttpHtmlRootEnd - roverHttpHtmlRootStart;

	ESP_LOGI( roverLogTAG, "Serve root %d", req->method );

	if ( HTTP_GET == req->method ) {
		httpd_resp_set_type( req, "text/html" );
		httpd_resp_send( req, roverHttpHtmlRootStart, root_len );
	}
	else if ( HTTP_POST == req->method ) {
		char content[req->content_len + 1];

		if ( httpd_req_recv( req, content, req->content_len ) > 0 ) {
			content[req->content_len] = '\0';
			ESP_LOGI( roverLogTAG, "Body: %s", content );

			char ssid[32 * 3];

			if ( httpd_query_key_value( content, "ssid", ssid, sizeof ssid ) != ESP_OK ) {
				goto l_exit;
			}

			char password[64 * 3];

			if ( httpd_query_key_value( content, "password", password, sizeof password ) != ESP_OK ) {
				goto l_exit;
			}

			if ( roverHttpHandlerPostWlanConfig != NULL ) {
				roverHttpHandlerPostWlanConfig( rover_uri_unescape( ssid ), rover_uri_unescape( password ) );
			}
		}
	}

l_exit:
	return ESP_OK;
}


esp_err_t rover_http_404_error_handler( httpd_req_t * req, httpd_err_code_t err )
{
	// Set status
	httpd_resp_set_status( req, "302 Temporary Redirect" );
	// Redirect to the "/" root directory
	httpd_resp_set_hdr( req, "Location", "/" );
	// iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
	httpd_resp_send( req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN );

	ESP_LOGI( roverLogTAG, "Redirecting to root" );
	return ESP_OK;
}
