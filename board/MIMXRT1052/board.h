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

#ifndef _BOARD_H_
#define _BOARD_H_

#include "clock_config.h"
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"

// =======================================================
// ===== The board name, define ONLY ONE =================
// =======================================================

// The only differenced between board are pins
// used for LED and user button

#define BOARD_IMXRT1050_EVKB
//#define BOARD_SOMLABS
//#define BOARD_ARCHMIX

// =======================================================

// === The USER_LED used for board ===
#ifdef BOARD_ARCHMIX
#define BOARD_NAME "SEEED ARCH-MIX"

//-------------------------------
// LED used for status indication
//-------------------------------
#define BOARD_USER_LED_OFF_POLARITY	1U
#define BOARD_USER_LED_ON_POLARITY	0U
#define BOARD_USER_LED_PIN IOMUXC_GPIO_AD_B0_11_GPIO1_IO11 // BLUE LED
#define IMRXT_BA_LED_GPIO  GPIO1
#define IMRXT_BA_LED_GPIO_PIN (11U)
// unused LED, will be turned of on start
// this can be the default LED initialized and used by ROM bootloader
#define BOARD_USER_UNUSED_LED_PIN IOMUXC_GPIO_AD_B0_10_GPIO1_IO10 // disable GREEN LED
#define IMRXT_BA_UNUSED_LED_GPIO  GPIO1
#define IMRXT_BA_UNUSED_LED_GPIO_PIN (10U)

//----------------------------------------
// User button used to activate bootloader
// when valid boot application exists
//----------------------------------------
//#define BOARD_USER_BUTTON_PIN	IOMUXC_SNVS_WAKEUP_GPIO5_IO00 // USER BUTTON
#define BOARD_USER_BUTTON_GPIO GPIO5
#define BOARD_USER_BUTTON_GPIO_PIN (0U)

#else // !BOARD_ARCHMIX

#ifdef BOARD_SOMLABS
#define BOARD_NAME "SomLabs VisionCB-RT-STD"

//-------------------------------
// LED used for status indication
//-------------------------------
#define BOARD_USER_LED_OFF_POLARITY	0U
#define BOARD_USER_LED_ON_POLARITY	1U
#define BOARD_USER_LED_PIN IOMUXC_GPIO_AD_B0_08_GPIO1_IO08 // BLUE LED
#define IMRXT_BA_LED_GPIO  GPIO1
#define IMRXT_BA_LED_GPIO_PIN (8U)
// unused LED, will be turned of on start
// this can be the default LED initialized and used by ROM bootloader
#define BOARD_USER_UNUSED_LED_PIN IOMUXC_GPIO_AD_B0_09_GPIO1_IO09 // disable RED LED
#define IMRXT_BA_UNUSED_LED_GPIO  GPIO1
#define IMRXT_BA_UNUSED_LED_GPIO_PIN (9U)
// unused LED, will be turned of on start
// this can be the default LED initialized and used by ROM bootloader
#define BOARD_USER_UNUSED1_LED_PIN IOMUXC_GPIO_AD_B0_06_GPIO1_IO06 // disable GREENLED
#define IMRXT_BA_UNUSED1_LED_GPIO  GPIO1
#define IMRXT_BA_UNUSED1_LED_GPIO_PIN (6U)

//----------------------------------------
// User button used to activate bootloader
// when valid boot application exists
//----------------------------------------
#define BOARD_USER_BUTTON_PIN IOMUXC_GPIO_AD_B0_03_GPIO1_IO03 // First button (IO3)
#define BOARD_USER_BUTTON_GPIO GPIO1
#define BOARD_USER_BUTTON_GPIO_PIN (3U)

#else // !BOARD_SOMLABS
#define BOARD_NAME "IMXRT1050-EVKB"

//-------------------------------
// LED used for status indication
//-------------------------------
#define BOARD_USER_LED_OFF_POLARITY	1U
#define BOARD_USER_LED_ON_POLARITY	0U
#define BOARD_USER_LED_PIN IOMUXC_GPIO_AD_B0_09_GPIO1_IO09 // USER LED(D18)
#define IMRXT_BA_LED_GPIO  GPIO1
#define IMRXT_BA_LED_GPIO_PIN (9U)

//----------------------------------------
// User button used to activate bootloader
// when valid boot application exists
//----------------------------------------
//#define BOARD_USER_BUTTON_PIN	IOMUXC_SNVS_WAKEUP_GPIO5_IO00 // SW0
#define BOARD_USER_BUTTON_GPIO GPIO5
#define BOARD_USER_BUTTON_GPIO_PIN (0U)

#endif // BOARD_SOMLABS

#endif // BOARD_ARCHMIX

// The QuadSPI flash size
#define BOARD_FLASH_SIZE (0x08000000U)

// USB PHY condfiguration
#define BOARD_USB_PHY_D_CAL (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)

void BOARD_ConfigMPU(void);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif // _BOARD_H_
