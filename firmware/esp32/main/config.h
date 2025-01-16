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

#ifndef __ROVER__CONFIG__H
#define __ROVER__CONFIG__H


#include <stdint.h>
#include <stddef.h>


typedef struct {
	const char * ssid;
	const char * password;
} t_rover_config_wlan;

typedef struct {
	t_rover_config_wlan wlan;
} t_rover_config;


bool rover_load_config( t_rover_config * out );
void rover_save_config( const t_rover_config * config );
void rover_reset_config( void );


#endif
