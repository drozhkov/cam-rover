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

#ifndef __ROVER__DRIVE__H
#define __ROVER__DRIVE__H


#include <stdbool.h>

#include "bdc_motor.h"


typedef struct {
	uint32_t gpioNumA;
	uint32_t gpioNumB;
} t_rover_drive_motor_gpio;

typedef struct {
	bdc_motor_handle_t handle;
	int32_t speed;
	t_rover_drive_motor_gpio gpio;
} t_rover_drive_motor;

typedef struct {
	uint32_t freqHz;
	uint32_t timerResolutionHz;
	uint32_t dutyTickMax;
} t_rover_drive_pwm;

typedef struct {
	t_rover_drive_motor motor1;
	t_rover_drive_motor motor2;
	t_rover_drive_pwm pwm;
	uint32_t deadzone;
	uint32_t * speedCurve;
} t_rover_drive;


void rover_drive_init( t_rover_drive * drive );
int32_t rover_drive_change_motor_speed(
	t_rover_drive * drive, t_rover_drive_motor * motor, int32_t speedInc, bool isDirectMode );
	
t_rover_motors_speed rover_drive_change_speed(
	t_rover_drive * drive, int32_t motor1SpeedInc, int32_t motor2SpeedInc, bool isDirectMode );


#endif
