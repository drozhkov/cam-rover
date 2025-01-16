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

#ifndef __ROVER__COMM_UDP__H
#define __ROVER__COMM_UDP__H


#include <time.h>

#include "freertos/semphr.h"

#include "lwip/sockets.h"

#include "types.h"


typedef struct {
	int socketFd;
	StaticSemaphore_t syncBuffer;
	SemaphoreHandle_t sync;
	struct sockaddr clientAddress;
	t_rover_comm_handlers handlers;
	struct timespec lastReceiveTs;
	uint16_t portNo;
} t_rover_comm_udp;


void rover_comm_udp_send( t_rover_comm_udp * commUdp, uint8_t * data, size_t dataLen );
uint16_t rover_comm_udp_start( t_rover_comm_udp * commUdp );


#endif
