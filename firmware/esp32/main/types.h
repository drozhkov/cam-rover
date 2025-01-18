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

#ifndef __ROVER__TYPES__H
#define __ROVER__TYPES__H


#include <stddef.h>
#include <stdint.h>


#define ROVER_CALL( a_handler, ... )                                                                                   \
	if ( a_handler != NULL ) {                                                                                         \
		a_handler( __VA_ARGS__ );                                                                                      \
	}

#define ROVER_CALL_FUNC( a_handler, a_result, ... )                                                                    \
	if ( a_handler != NULL ) {                                                                                         \
		a_result = a_handler( __VA_ARGS__ );                                                                           \
	}

#define ROVER_CAMER_SET_DEFAULT( a_field, a_default )                                                                  \
	if ( 0 == ( a_field ) ) {                                                                                          \
		( a_field ) = a_default;                                                                                       \
	}


typedef struct {
	uint8_t * data;
	size_t size;
	size_t len;
	uintptr_t pos;
} t_rover_buffer;

typedef struct {
	void * data;
	void * owner;
	size_t size;
	size_t refCount;
} t_rover_ptr;

typedef struct {
	int32_t motor1;
	int32_t motor2;
} t_rover_motors_speed;

typedef t_rover_motors_speed ( *t_rover_comm_handler_move_speed )( int32_t inc );
typedef void ( *t_rover_comm_handler_move_stop )( void );
typedef t_rover_motors_speed ( *t_rover_comm_handler_move_turn )( int32_t incL, int32_t incR );
typedef t_rover_motors_speed ( *t_rover_comm_handler_move_set )( int32_t speedL, int32_t speedR );
typedef void ( *t_rover_comm_handler_move_deadzone )( uint32_t v );

typedef struct {
	t_rover_comm_handler_move_speed speed;
	t_rover_comm_handler_move_stop stop;
	t_rover_comm_handler_move_turn turn;
	t_rover_comm_handler_move_set set;
	t_rover_comm_handler_move_deadzone deadzone;
} t_rover_comm_move_handlers;

typedef void ( *t_rover_comm_handler_camera_flash )( uint8_t duty );

typedef struct {
	t_rover_comm_handler_camera_flash flash;
} t_rover_comm_camera_handlers;

typedef void ( *t_rover_comm_handler_camera_flash )( uint8_t duty );

typedef struct {
	t_rover_comm_move_handlers move;
	t_rover_comm_camera_handlers camera;
} t_rover_comm_handlers;


#endif
