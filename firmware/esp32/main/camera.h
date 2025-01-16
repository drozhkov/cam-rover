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

#ifndef __ROVER__CAMERA__H
#define __ROVER__CAMERA__H


#include "esp_camera.h"


typedef struct {
	uint8_t * ptr;
	size_t len;
} t_rover_camera_frame;

typedef void ( *t_rover_camera_handler_frame )( t_rover_camera_frame * );

typedef struct {
	ledc_mode_t ledcMode;
	ledc_channel_t ledcChannel;
} t_rover_camera_flash;

typedef struct {
	camera_config_t config;
	t_rover_camera_handler_frame frameHandler;
	t_rover_camera_flash flash;
} t_rover_camera;


void rover_camera_set_flash_duty( t_rover_camera * camera, uint32_t duty );
void rover_camera_start( t_rover_camera * camera );


#endif
