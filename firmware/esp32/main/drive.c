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
#include "drive.h"


#define ROVER_DRIVE_MCPWM_TIMER_RESOLUTION_HZ ( 1000 * 1000 )
#define ROVER_DRIVE_MCPWM_FREQ_HZ ( 10 * 1000 )

#define ROVER_DRIVE_MCPWM_GPIO1_A 13
#define ROVER_DRIVE_MCPWM_GPIO1_B 15
#define ROVER_DRIVE_MCPWM_GPIO2_A 14
#define ROVER_DRIVE_MCPWM_GPIO2_B 2


static const char * roverLogTAG = "rover.drive";


static void rover_drive_init_motor_config( bdc_motor_config_t * config,
	t_rover_drive_pwm * pwm,
	t_rover_drive_motor_gpio * gpio,
	uint32_t gpioA,
	uint32_t gpioB )
{

	ROVER_CAMER_SET_DEFAULT( pwm->freqHz, ROVER_DRIVE_MCPWM_FREQ_HZ );
	ROVER_CAMER_SET_DEFAULT( gpio->gpioNumA, gpioA );
	ROVER_CAMER_SET_DEFAULT( gpio->gpioNumB, gpioB );

	config->pwm_freq_hz = pwm->freqHz;
	config->pwma_gpio_num = gpio->gpioNumA;
	config->pwmb_gpio_num = gpio->gpioNumB;
}


void rover_drive_init( t_rover_drive * drive )
{
	bdc_motor_config_t motor1Config;
	rover_drive_init_motor_config(
		&motor1Config, &drive->pwm, &drive->motor1.gpio, ROVER_DRIVE_MCPWM_GPIO1_A, ROVER_DRIVE_MCPWM_GPIO1_B );

	bdc_motor_config_t motor2Config;
	rover_drive_init_motor_config(
		&motor2Config, &drive->pwm, &drive->motor2.gpio, ROVER_DRIVE_MCPWM_GPIO2_A, ROVER_DRIVE_MCPWM_GPIO2_B );

	ROVER_CAMER_SET_DEFAULT( drive->pwm.timerResolutionHz, ROVER_DRIVE_MCPWM_TIMER_RESOLUTION_HZ );

	bdc_motor_mcpwm_config_t mcpwmConfig = { .group_id = 0, .resolution_hz = drive->pwm.timerResolutionHz };

	drive->pwm.dutyTickMax = drive->pwm.timerResolutionHz / drive->pwm.freqHz;
	drive->speedCurve = malloc( ( drive->pwm.dutyTickMax + 1 ) * sizeof( uint32_t ) );

	{
		size_t limit = drive->pwm.dutyTickMax;

		for ( size_t i = 0; i < limit + 1; ++i ) {
			drive->speedCurve[i] = limit - ( ( limit - i ) * ( limit - i ) / ( 1 * limit ) );
		}

		drive->deadzone = limit / 3;
	}

	bdc_motor_handle_t motor1 = NULL;
	ESP_ERROR_CHECK( bdc_motor_new_mcpwm_device( &motor1Config, &mcpwmConfig, &motor1 ) );
	drive->motor1.handle = motor1;

	bdc_motor_handle_t motor2 = NULL;
	ESP_ERROR_CHECK( bdc_motor_new_mcpwm_device( &motor2Config, &mcpwmConfig, &motor2 ) );
	drive->motor2.handle = motor2;

	bdc_motor_set_speed( motor1, 0 );
	bdc_motor_set_speed( motor2, 0 );

	ESP_ERROR_CHECK( bdc_motor_enable( motor1 ) );
	ESP_ERROR_CHECK( bdc_motor_enable( motor2 ) );

	ESP_ERROR_CHECK( bdc_motor_forward( motor1 ) );
	ESP_ERROR_CHECK( bdc_motor_forward( motor2 ) );
}


int32_t rover_drive_change_motor_speed( t_rover_drive * drive, t_rover_drive_motor * motor, int32_t speedInc )
{
	int32_t speed = motor->speed;

	if ( abs( speed + speedInc ) > drive->pwm.dutyTickMax ) {
		motor->speed = drive->pwm.dutyTickMax;
	}
	else {
		motor->speed += speedInc;
	}

	// ESP_LOGI( roverLogTAG, "set motor speed: %d, %d", (int)speed, (int)motor->speed );
	uint32_t actualSpeed = motor->speed != 0 ? ( drive->speedCurve[abs( motor->speed )] + drive->deadzone ) : 0;

	if ( speed == motor->speed ) {
		goto _l_exit;
	}

	if ( actualSpeed > drive->pwm.dutyTickMax ) {
		actualSpeed = drive->pwm.dutyTickMax;
	}

	if ( speed >= 0 && motor->speed < 0 ) {
		bdc_motor_reverse( motor->handle );
	}
	else if ( speed < 0 && motor->speed >= 0 ) {
		bdc_motor_forward( motor->handle );
	}

	bdc_motor_set_speed( motor->handle, actualSpeed );

_l_exit:
	return ( actualSpeed * ( motor->speed < 0 ? -1 : 1 ) );
}


t_rover_motors_speed rover_drive_change_speed( t_rover_drive * drive, int32_t motor1SpeedInc, int32_t motor2SpeedInc )
{
	t_rover_motors_speed r;
	r.motor1 = rover_drive_change_motor_speed( drive, &drive->motor1, motor1SpeedInc );
	r.motor2 = rover_drive_change_motor_speed( drive, &drive->motor2, motor2SpeedInc );
	return r;
}
