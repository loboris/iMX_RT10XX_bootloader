/**
 * The MIT License (MIT)
 * 
 * Part of the iMX RT MicroPython port
 * iMX RT CDC ACM Bootloader with OTA support
 *
 * Code inspired by CDC Arduino bootloader for SeeedStudio's ArchMix board
 * https://github.com/Seeed-Studio/ArduinoCore-imxrt/tree/master/bootloaders
 * 
 * Author: LoBo (loboris@gmail.com)
 * 
 * Copyright (C) 2021  LoBo
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "app.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "board_drive_led.h"

static uint32_t led_state = BOARD_USER_LED_OFF_POLARITY;

void LED_init(void) 
{
	gpio_pin_config_t led_config = {kGPIO_DigitalOutput, 1, kGPIO_NoIntmode};

	#ifdef BOARD_USER_UNUSED_LED_PIN
	IOMUXC_SetPinMux(BOARD_USER_UNUSED_LED_PIN, 0U);
  IOMUXC_SetPinConfig(BOARD_USER_UNUSED_LED_PIN, 0x10B0U);

	GPIO_PinInit(IMRXT_BA_UNUSED_LED_GPIO, IMRXT_BA_UNUSED_LED_GPIO_PIN, &led_config);
	GPIO_PinWrite(IMRXT_BA_UNUSED_LED_GPIO, IMRXT_BA_UNUSED_LED_GPIO_PIN, BOARD_USER_LED_OFF_POLARITY);
	#endif

	#ifdef BOARD_USER_UNUSED1_LED_PIN
	IOMUXC_SetPinMux(BOARD_USER_UNUSED1_LED_PIN, 0U);
  IOMUXC_SetPinConfig(BOARD_USER_UNUSED1_LED_PIN, 0x10B0U);

	GPIO_PinInit(IMRXT_BA_UNUSED1_LED_GPIO, IMRXT_BA_UNUSED1_LED_GPIO_PIN, &led_config);
	GPIO_PinWrite(IMRXT_BA_UNUSED1_LED_GPIO, IMRXT_BA_UNUSED1_LED_GPIO_PIN, BOARD_USER_LED_OFF_POLARITY);
	#endif

	IOMUXC_SetPinMux(BOARD_USER_LED_PIN, 0U);
  /* PAD functional properties :
         Slew Rate Field: Slow Slew Rate
				 Drive Strength Field: R0/6
				 Speed Field: medium(100MHz)
				 Open Drain Enable Field: Open Drain Disabled
				 Pull / Keep Enable Field: Pull/Keeper Enabled
				 Pull / Keep Select Field: Keeper
				 Pull Up / Down Config. Field: 100K Ohm Pull Down
				 Hyst. Enable Field: Hysteresis Disabled
	*/
  IOMUXC_SetPinConfig(BOARD_USER_LED_PIN, 0x10B0U);

	GPIO_PinInit(IMRXT_BA_LED_GPIO, IMRXT_BA_LED_GPIO_PIN, &led_config);
	GPIO_PinWrite(IMRXT_BA_LED_GPIO, IMRXT_BA_LED_GPIO_PIN, BOARD_USER_LED_OFF_POLARITY);
	led_state = BOARD_USER_LED_OFF_POLARITY;
}

void LED_on(void)
{
	GPIO_PinWrite(IMRXT_BA_LED_GPIO, IMRXT_BA_LED_GPIO_PIN, BOARD_USER_LED_ON_POLARITY);
	led_state = BOARD_USER_LED_ON_POLARITY;
}

void LED_off(void)
{
	GPIO_PinWrite(IMRXT_BA_LED_GPIO, IMRXT_BA_LED_GPIO_PIN, BOARD_USER_LED_OFF_POLARITY);
	led_state = BOARD_USER_LED_OFF_POLARITY;
}

void LED_toggle(void)
{
	led_state ^= 1;
	GPIO_PinWrite(IMRXT_BA_LED_GPIO, IMRXT_BA_LED_GPIO_PIN, led_state);
}
