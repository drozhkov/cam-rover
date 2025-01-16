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

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "helpers.h"


void rover_to_hex( char * buffer, uint8_t * data, size_t len )
{
	static char hex[] = "01234567890abcdef";

	for ( size_t i = 0; i < len; ++i ) {
		buffer[i * 2] = hex[data[i] >> 4];
		buffer[i * 2 + 1] = hex[data[i] & 0x0f];
	}

	buffer[len * 2] = '\0';
}


const char * rover_uri_unescape( char * s )
{
	size_t len = strlen( s );
	bool isEscapedChar = false;
	size_t targetIndex = 0;
	char escapedCharHex[3];
	size_t escapedCharHexIndex = 0;

	for ( size_t i = 0; i < len; ++i ) {
		if ( '%' == s[i] ) {
			isEscapedChar = true;
		}
		else if ( isEscapedChar ) {
			escapedCharHex[escapedCharHexIndex++] = s[i];

			if ( escapedCharHexIndex > 1 ) {
				escapedCharHex[escapedCharHexIndex] = '\0';

				s[targetIndex++] = strtoul( escapedCharHex, NULL, 16 );

				escapedCharHexIndex = 0;
				isEscapedChar = false;
			}
		}
		else {
			s[targetIndex++] = s[i];
		}
	}

	s[targetIndex] = '\0';

	return s;
}
