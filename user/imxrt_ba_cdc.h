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

#ifndef _IMRXT_BA_USB_CDC_H_
#define _IMRXT_BA_USB_CDC_H_

#include <stdint.h>
#include <stdbool.h>
#include "virtual_com.h"

#define CDC_READBUF_TIMEOUT 	250

extern usb_cdc_vcom_struct_t s_cdcVcom;	
extern uint32_t CPUFreq;
extern char log_data[];
extern uint32_t log_data_ptr;

/**
 * \brief Sends a single byte through USB CDC
 *
 * \param Data to send
 * \return number of data sent
 */
int cdc_putc(int value);

/**
 * \brief Reads a single byte through USB CDC
 *
 * \return Data read through USB
 */
int cdc_getc(void);

/**
 * \brief Checks if a character has been received on USB CDC
 *
 * \return \c 1 if a byte is ready to be read.
 */
bool cdc_is_rx_ready(void);

/**
 * \brief Sends buffer on USB CDC
 *
 * \param data pointer
 * \param number of data to send
 * \return number of data sent
 */
uint32_t cdc_write_buf(void const* data, uint32_t length);

/**
 * \brief Gets specified number of bytes on USB CDC
 *
 * \param data pointer
 * \param number of data to read
 * \param timeout in mili seconds
 * \return number of data read
 */
uint32_t cdc_read_buf(void* data, uint32_t length, uint32_t timeout);

void print(const char* format, ...);
void print_hex(const char* buf, uint32_t length, bool split);
void cdc_rx_ignore(void);
void log_print(const char* format, ...);
void byte_to_hex(uint8_t c, char *hexBuf);

#endif // _IMRXT_BA_USB_CDC_H_
