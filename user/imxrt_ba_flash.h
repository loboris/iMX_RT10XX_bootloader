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
 
#ifndef _IMRXT_BA_FLASH_H
#define _IMRXT_BA_FLASH_H

#include "fsl_common.h"

#define FERR_OK							kStatus_Success
#define FERR_ADDRESS_ALIGN	99
#define FERR_ADDRESS_MIN		98
#define FERR_ADDRESS_MAX		97
#define FERR_ERASE					96
#define FERR_PROGRAM_PAGE		95
#define FERR_PROGRAM_BUFFER	94
#define FERR_LENGTH					93

status_t flash_erase(uint32_t address, uint32_t length);
status_t flash_program_page(uint32_t address, void * data);
status_t flash_program_buffer(uint32_t address, uint8_t *data, uint32_t length);
void flash_read(uint32_t address, void * data,const uint32_t length);
int _sector_erased(uint32_t address);
int _check_flash_data(uint32_t address, uint8_t * data, uint32_t length);

#endif /*IMRXT_BA_FLASH_H*/
