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

#ifndef __ROVER__COMM__H
#define __ROVER__COMM__H


#include "types.h"


#define ROVER_COMM_U32( a_message, a_index )                                                                           \
	( ( (uint32_t)( a_message )[a_index] ) | ( (uint32_t)( a_message )[a_index + 1] << 8 ) |                           \
		( (uint32_t)( a_message )[a_index + 2] << 16 ) | ( (uint32_t)( a_message )[a_index + 3] << 24 ) )

#define ROVER_COMM_MESSAGE_PAYLOAD_LEN( a_message ) ( ( a_message )[0] )
#define ROVER_COMM_MESSAGE_ID( a_message ) ROVER_COMM_U32( a_message, 1 )
#define ROVER_COMM_MESSAGE_COMMAND( a_message ) ( ( a_message )[5] )
#define ROVER_COMM_MESSAGE_MOVE_SPEED( a_message ) ( ( a_message )[6] )
#define ROVER_COMM_MESSAGE_MOVE_SPEED_L( a_message ) ( (int32_t)ROVER_COMM_U32( a_message, 6 ) )
#define ROVER_COMM_MESSAGE_MOVE_SPEED_R( a_message ) ( (int32_t)ROVER_COMM_U32( a_message, 10 ) )
#define ROVER_COMM_MESSAGE_CAMERA_FLASH_DUTY( a_message ) ( ( a_message )[6] )


typedef enum {
	ROVER_COMM_COMMAND_UNKNOWN = 0,
	ROVER_COMM_COMMAND_MOVE_SPEED_UP = '+',
	ROVER_COMM_COMMAND_MOVE_SPEED_DOWN = '-',
	ROVER_COMM_COMMAND_MOVE_TURN_LEFT = 'l',
	ROVER_COMM_COMMAND_MOVE_TURN_RIGHT = 'r',
	ROVER_COMM_COMMAND_MOVE_STOP = 's',
	ROVER_COMM_COMMAND_MOVE_SET = 't',
	ROVER_COMM_COMMAND_CAMERA_FLASH = 'f',
	ROVER_COMM_COMMAND_ACK = 'a'
} t_rover_comm_command;


inline void rover_comm_message_serialzie_u32( t_rover_buffer * message, uint32_t v )
{
	message->data[message->pos++] = v & 0xff;
	message->data[message->pos++] = ( v >> 8 ) & 0xff;
	message->data[message->pos++] = ( v >> 16 ) & 0xff;
	message->data[message->pos++] = ( v >> 24 ) & 0xff;
	message->len = message->pos;
}


inline void rover_comm_message_serialzie_id( t_rover_buffer * message, uint32_t messageId )
{
	rover_comm_message_serialzie_u32( message, messageId );
}


inline void rover_comm_message_init(
	t_rover_buffer * message, uint8_t * buffer, t_rover_comm_command cmd, uint32_t messageId )
{

	message->pos = 1;
	message->data = buffer;
	rover_comm_message_serialzie_id( message, messageId );
	message->data[message->pos++] = cmd;
}


inline void rover_comm_message_update_payload_len( t_rover_buffer * message )
{
	message->data[0] = message->len - 1;
}


#endif
