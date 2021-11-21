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

#ifndef _MONITOR_IMRXT_BA_H_
#define _MONITOR_IMRXT_BA_H_

#include <stdio.h>
#include <stdint.h>
#include "app.h"

// Binary command constants
#define CMD_GET_VERSION							0x0000D001
#define CMD_READ_FLASH							0x0000D102
#define CMD_WRITE_FLASH							0x0000D103
#define CMD_APP_RECORD_READ					0x0000D204
#define CMD_APP_RECORD_WRITE				0x0000D205
#define CMD_APP_GETSHA256						0x0000D206

// Command error codes
#define CMD_ERR_OK									0x00000000
#define CMD_ERR_CRC									0x0000E101
#define CMD_ERR_FLASH_WRITE					0x0000E102
#define CMD_ERR_UNKNOWN_CMD					0x0000E103
#define CMD_ERR_ADDRESS							0x0000E104
#define CMD_ERR_LENGTH							0x0000E105
#define CMD_ERR_DATA								0x0000E106
#define CMD_ERR_DATACRC							0x0000E107
#define CMD_ERR_APPREC_READ					0x0000E108
#define CMD_ERR_SHA256							0x0000E109
#define CMD_ERR_BOOTREC_READ				0x0000E10A
#define CMD_ERR_BOOTREC_WRITE				0x0000E10B
#define CMD_ERR_BKPBOOTREC_WRITE		0x0000E10C
#define CMD_ERR_FLASHDATACRC				0x0000E10D
#define CMD_ERR_FLASHERASE					0x0000E10E

// Other definitions
#define DATA_BLOCK_SIZE		4096
#define CMD_SIZE					20
#define CMD_SIZE_BASE			16

// Binary command structure
//--------------------------
typedef struct _command_t_ {
	uint32_t cmd;
	uint32_t param;
	uint32_t data_len;
	uint32_t data_crc;
	uint32_t crc;
	uint8_t  cmd_data[DATA_BLOCK_SIZE+256];
}	command_t;


//uint16_t crc16(const void* data, size_t length, uint16_t previousCrc16);
uint32_t crc32(const void* data, size_t length, uint32_t previousCrc32);

// Main function of the bootloader monitor
void imxrt_ba_monitor_run(void);

#endif // _MONITOR_IMRXT_BA_H_
