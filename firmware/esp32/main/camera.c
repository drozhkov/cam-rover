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

#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_camera.h"

#include "nvs_flash.h"

#include "types.h"
#include "camera.h"


#define ROVER_CAMERA_PIN_PWDN 32
#define ROVER_CAMERA_PIN_RESET -1 // software reset will be performed
#define ROVER_CAMERA_PIN_XCLK 0
#define ROVER_CAMERA_PIN_SIOD 26
#define ROVER_CAMERA_PIN_SIOC 27
#define ROVER_CAMERA_PIN_D7 35
#define ROVER_CAMERA_PIN_D6 34
#define ROVER_CAMERA_PIN_D5 39
#define ROVER_CAMERA_PIN_D4 36
#define ROVER_CAMERA_PIN_D3 21
#define ROVER_CAMERA_PIN_D2 19
#define ROVER_CAMERA_PIN_D1 18
#define ROVER_CAMERA_PIN_D0 5
#define ROVER_CAMERA_PIN_VSYNC 25
#define ROVER_CAMERA_PIN_HREF 23
#define ROVER_CAMERA_PIN_PCLK 22
#define ROVER_CAMERA_XCLK_FREQ_HZ ( 20 * 1000 * 1000 )


static const char * roverLogTAG = "rover.camera";

#define ROVER_CAMERA_FLASH_LEDC_TIMER LEDC_TIMER_1
#define ROVER_CAMERA_FLASH_LEDC_MODE LEDC_LOW_SPEED_MODE
#define ROVER_CAMERA_FLASH_LEDC_CHANNEL LEDC_CHANNEL_1
#define ROVER_CAMERA_FLASH_LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define ROVER_CAMERA_FLASH_LEDC_FREQ_HZ ( 5000 )
#define ROVER_CAMERA_FLASH_GPIO ( 4 ) // Define the output GPIO


static void rover_camera_flash_led_init( t_rover_camera_flash * flash )
{
	ROVER_CAMER_SET_DEFAULT( flash->ledcChannel, ROVER_CAMERA_FLASH_LEDC_CHANNEL );
	ROVER_CAMER_SET_DEFAULT( flash->ledcMode, ROVER_CAMERA_FLASH_LEDC_MODE );

	// Prepare and then apply the LEDC PWM timer configuration
	ledc_timer_config_t timerConfig = { .speed_mode = flash->ledcMode,
		.duty_resolution = ROVER_CAMERA_FLASH_LEDC_DUTY_RES,
		.timer_num = ROVER_CAMERA_FLASH_LEDC_TIMER,
		.freq_hz = ROVER_CAMERA_FLASH_LEDC_FREQ_HZ,
		.clk_cfg = LEDC_AUTO_CLK };

	ESP_ERROR_CHECK( ledc_timer_config( &timerConfig ) );

	// Prepare and then apply the LEDC PWM channel configuration
	ledc_channel_config_t channelConfig = { .speed_mode = flash->ledcMode,
		.channel = flash->ledcChannel,
		.timer_sel = ROVER_CAMERA_FLASH_LEDC_TIMER,
		.intr_type = LEDC_INTR_DISABLE,
		.gpio_num = ROVER_CAMERA_FLASH_GPIO,
		.duty = 0, // Set duty to 0%
		.hpoint = 0 };

	ESP_ERROR_CHECK( ledc_channel_config( &channelConfig ) );
}


static esp_err_t rover_camera_init( camera_config_t * config )
{
	ROVER_CAMER_SET_DEFAULT( config->pin_pwdn, ROVER_CAMERA_PIN_PWDN );
	ROVER_CAMER_SET_DEFAULT( config->pin_reset, ROVER_CAMERA_PIN_RESET );
	ROVER_CAMER_SET_DEFAULT( config->pin_xclk, ROVER_CAMERA_PIN_XCLK );
	ROVER_CAMER_SET_DEFAULT( config->pin_sccb_sda, ROVER_CAMERA_PIN_SIOD );
	ROVER_CAMER_SET_DEFAULT( config->pin_sccb_scl, ROVER_CAMERA_PIN_SIOC );
	ROVER_CAMER_SET_DEFAULT( config->pin_d7, ROVER_CAMERA_PIN_D7 );
	ROVER_CAMER_SET_DEFAULT( config->pin_d6, ROVER_CAMERA_PIN_D6 );
	ROVER_CAMER_SET_DEFAULT( config->pin_d5, ROVER_CAMERA_PIN_D5 );
	ROVER_CAMER_SET_DEFAULT( config->pin_d4, ROVER_CAMERA_PIN_D4 );
	ROVER_CAMER_SET_DEFAULT( config->pin_d3, ROVER_CAMERA_PIN_D3 );
	ROVER_CAMER_SET_DEFAULT( config->pin_d2, ROVER_CAMERA_PIN_D2 );
	ROVER_CAMER_SET_DEFAULT( config->pin_d1, ROVER_CAMERA_PIN_D1 );
	ROVER_CAMER_SET_DEFAULT( config->pin_d0, ROVER_CAMERA_PIN_D0 );
	ROVER_CAMER_SET_DEFAULT( config->pin_vsync, ROVER_CAMERA_PIN_VSYNC );
	ROVER_CAMER_SET_DEFAULT( config->pin_href, ROVER_CAMERA_PIN_HREF );
	ROVER_CAMER_SET_DEFAULT( config->pin_pclk, ROVER_CAMERA_PIN_PCLK );
	ROVER_CAMER_SET_DEFAULT( config->xclk_freq_hz, ROVER_CAMERA_XCLK_FREQ_HZ );
	ROVER_CAMER_SET_DEFAULT( config->ledc_timer, LEDC_TIMER_0 );
	ROVER_CAMER_SET_DEFAULT( config->ledc_channel, LEDC_CHANNEL_0 );
	ROVER_CAMER_SET_DEFAULT( config->pixel_format, PIXFORMAT_JPEG );
	ROVER_CAMER_SET_DEFAULT( config->frame_size, FRAMESIZE_VGA );
	ROVER_CAMER_SET_DEFAULT( config->jpeg_quality, 12 );
	ROVER_CAMER_SET_DEFAULT( config->fb_count, 2 );
	ROVER_CAMER_SET_DEFAULT( config->fb_location, CAMERA_FB_IN_PSRAM );
	ROVER_CAMER_SET_DEFAULT( config->grab_mode, CAMERA_GRAB_WHEN_EMPTY );

	esp_err_t err = esp_camera_init( config );

	if ( err != ESP_OK ) {
		ESP_LOGE( roverLogTAG, "Camera Init Failed" );
		return err;
	}

	return ESP_OK;
}


static void rover_camera_task( void * parameters )
{
	t_rover_camera * camera = (t_rover_camera *)parameters;

	clock_t startTs = 0;
	size_t frameCount = 0;

	while ( true ) {
		if ( 0 == startTs ) {
			startTs = clock();
			frameCount = 0;
		}

		// ESP_LOGI( TAG, "Taking picture..." );
		camera_fb_t * pic = esp_camera_fb_get();

		// use pic->buf to access the image
		// ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
		ROVER_CALL( camera->frameHandler, &( t_rover_camera_frame ){ .ptr = pic->buf, .len = pic->len } );

		esp_camera_fb_return( pic );

		frameCount++;
		clock_t elapsed = clock() - startTs;

		if ( ( elapsed / CLOCKS_PER_SEC ) > 5 ) {
			ESP_LOGI( roverLogTAG, "FPS:  %d", (int)( frameCount * 1000 / elapsed ) );
			startTs = 0;
		}

		// vTaskDelay( 5000 / portTICK_RATE_MS );
	}
}


void rover_camera_set_flash_duty( t_rover_camera * camera, uint32_t duty )
{
	ledc_set_duty( camera->flash.ledcMode, camera->flash.ledcChannel, duty );
	ledc_update_duty( camera->flash.ledcMode, camera->flash.ledcChannel );
}


void rover_camera_start( t_rover_camera * camera )
{
	rover_camera_init( &camera->config );
	rover_camera_flash_led_init( &camera->flash );

	xTaskCreate( &rover_camera_task, "rover_camera_task", 4096, camera, 5, NULL );
}
