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

#include "types.h"
#include "globals.h"

#include "helpers.h"
#include "http.h"


extern const char roverHttpHtmlRootStart[] asm( "_binary_root_html_start" );
extern const char roverHttpHtmlRootEnd[] asm( "_binary_root_html_end" );

static t_rover_http_handler_post_wlan_config roverHttpHandlerPostWlanConfig = NULL;
static t_rover_http_handler_get_ssid_list roverHttpHandlerGetSsidList = NULL;

static const char * roverLogTAG = "rover.http";


void rover_http_set_handler_post_wlan_config( t_rover_http_handler_post_wlan_config handler )
{
	roverHttpHandlerPostWlanConfig = handler;
}


void rover_http_set_handler_get_ssid_list( t_rover_http_handler_get_ssid_list handler )
{
	roverHttpHandlerGetSsidList = handler;
}


esp_err_t rover_http_root_handler( httpd_req_t * req )
{
	const ssize_t rootHtmlLen = roverHttpHtmlRootEnd - roverHttpHtmlRootStart;

	ESP_LOGI( roverLogTAG, "Serve root %d %s", req->method, req->uri );

	if ( HTTP_GET == req->method ) {
		char action[32];

		size_t queryLen = httpd_req_get_url_query_len( req );

		if ( queryLen > 0 ) {
			char queryBuffer[queryLen + 1];
			httpd_req_get_url_query_str( req, queryBuffer, queryLen + 1 );
			ESP_LOGI( roverLogTAG, "query: %s", queryBuffer );

			if ( httpd_query_key_value( queryBuffer, "a", action, sizeof action ) == ESP_OK ) {
				bool isRequestHandled = false;

				if ( ROVER_IS_STRING_EQ( "get-ssid-list", action ) && roverHttpHandlerGetSsidList != NULL ) {
					httpd_resp_set_type( req, "application/json" );
					const char * responseJson = roverHttpHandlerGetSsidList();

					if ( responseJson != NULL ) {
						httpd_resp_send( req, responseJson, strlen( responseJson ) );
					}
					else {
						const char json[] = "['<ssid>']";
						httpd_resp_send( req, json, ( sizeof json ) - 1 );
					}

					isRequestHandled = true;
				}

				if ( isRequestHandled ) {
					goto _l_exit;
				}
			}
		}

		httpd_resp_set_type( req, "text/html" );
		httpd_resp_send( req, roverHttpHtmlRootStart, rootHtmlLen );
	}
	else if ( HTTP_POST == req->method ) {
		char content[req->content_len + 1];

		if ( httpd_req_recv( req, content, req->content_len ) > 0 ) {
			content[req->content_len] = '\0';
			ESP_LOGI( roverLogTAG, "Body: %s", content );

			char ssid[32 * 3];

			if ( httpd_query_key_value( content, "ssid", ssid, sizeof ssid ) != ESP_OK ) {
				goto _l_exit;
			}

			char password[64 * 3];

			if ( httpd_query_key_value( content, "password", password, sizeof password ) != ESP_OK ) {
				goto _l_exit;
			}

			ROVER_CALL( roverHttpHandlerPostWlanConfig, rover_uri_unescape( ssid ), rover_uri_unescape( password ) );
		}
	}

_l_exit:
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
