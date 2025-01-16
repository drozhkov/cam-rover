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

#ifndef __ROVER__HELPERS__H
#define __ROVER__HELPERS__H


#include <stdint.h>
#include <stddef.h>
#include <string.h>


#define ROVER_IS_STRING_EQ(a_s1, a_s2) (strcmp(a_s1, a_s2) == 0)
#define ROVER_STRING_STARTS_WITH(a_s1, a_s2) (strstr(a_s1, a_s2) == a_s1)


void rover_to_hex( char * buffer, uint8_t * data, size_t len );
const char * rover_uri_unescape( char * s );


#endif
