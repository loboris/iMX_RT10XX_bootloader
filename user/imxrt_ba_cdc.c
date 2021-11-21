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

#include <stdio.h>
#include <stdarg.h>
#include "imxrt_ba_cdc.h"

static char print_buf[256] = {0};
volatile static uint8_t cdc_rx_buff[1024];
volatile static uint32_t cdc_rx_buff_idx = 0;
char log_data[2048] = {0};
uint32_t log_data_ptr = 0;

//------------------------------
static void cdc_process_rx(void)
{
	if (cdc_is_rx_ready()) {
		uint32_t readed = vcom_read_buf((void *)(cdc_rx_buff+cdc_rx_buff_idx), 512);	
		if (readed) {
			cdc_rx_buff_idx += readed;
		}
	}
}

//---------------------
int cdc_putc(int value)
{
	if (!cdc_is_rx_ready())	return 0;

  return vcom_write_buf(&value, 1);
}

//----------------
int cdc_getc(void)
{
	if (!cdc_is_rx_ready()) return 0;
	uint8_t rx_char;

	cdc_process_rx();
	if (cdc_rx_buff_idx > 0) {
		rx_char = cdc_rx_buff[0];
		cdc_rx_buff_idx -= 1;
		if (cdc_rx_buff_idx > 0) {
			memmove((void *)cdc_rx_buff, (void *)(cdc_rx_buff+1), cdc_rx_buff_idx);
		}
	}
		
  return (int)rx_char;
}

//------------------------
bool cdc_is_rx_ready(void)
{
	if ((1 == s_cdcVcom.attach) && (1 == s_cdcVcom.startTransactions)) return true;
	else return false;
}

//-------------------------------------------------------
uint32_t cdc_write_buf(void const* data, uint32_t length)
{
	if (!cdc_is_rx_ready()) return 0;
	
	vcom_write_buf((void *)data, length);
  return length;
}

// read 'length' bytes from CDC input into 'data' buffer
//------------------------------------------------------------------
uint32_t cdc_read_buf(void* data, uint32_t length, uint32_t timeout)
{
	if (!cdc_is_rx_ready()) return 0;

  int32_t remaining = length;
	uint32_t readed = 0;
	uint32_t received = 0;
  char *dst = (char *)data;

	uint32_t tmo = timeout * CPUFreq;
	DWT->CYCCNT = 0;
  while (remaining > 0)
  {
		if (DWT->CYCCNT >tmo) break;
		// get new data from cdc input
		cdc_process_rx();
		if (cdc_rx_buff_idx > 0) {
			readed = 0;
			while (remaining > 0) {
				*dst = cdc_rx_buff[readed];
				dst++;
				remaining--;
				readed++;
				received++;
				if (readed >= cdc_rx_buff_idx) break;
			}
			cdc_rx_buff_idx -= readed;
			if (cdc_rx_buff_idx > 0) {
				memmove((void *)cdc_rx_buff, (void *)(cdc_rx_buff+readed), cdc_rx_buff_idx);
			}
		}
  }

  return received;
}

//----------------------
void cdc_rx_ignore(void)
{
  while (1)
  {
		cdc_process_rx();
		if (cdc_rx_buff_idx == 0) break;
		cdc_rx_buff_idx = 0;
  }
}

//---------------------------------------
void byte_to_hex(uint8_t c, char *hexBuf)
{
	char temp;
	temp = c & 0x0F;
	if (temp < 10) temp = temp + 48;
	else temp = temp + 55;
	hexBuf[1] = temp;
	temp = c >> 4;
	if (temp < 10) temp = temp + 48;
	else temp = temp + 55;
	hexBuf[0] = temp;
}

//----------------------------------------------------------
void print_hex(const char* buf, uint32_t length, bool split)
{
	char chex[4];
	uint32_t hlen = 2;
	if (split) {
		hlen = 3;
		cdc_write_buf("\r\n", 2);
	}
	for (int i=0; i<length; i++) {
		byte_to_hex(buf[i], chex);
		chex[2] = ' ';
		cdc_write_buf(chex, hlen);
		if ((split) && (i > 0) && ((i % 32) == 0)) {
			cdc_write_buf("\r\n", 2);
		}
	}
	if (split) {
		cdc_write_buf("\r\n", 2);
	}
}

//----------------------------------
void print(const char* format, ...)
{
  va_list va;
  va_start(va, format);
  int ret = vsprintf(print_buf, format, va);
  va_end(va);
	if (ret > 0) {
			cdc_write_buf(print_buf, ret);
	}
}

//-------------------------------------
void log_print(const char* format, ...)
{
  va_list va;
  va_start(va, format);
  int ret = vsprintf(print_buf, format, va);
  va_end(va);
	if ((ret > 0) && ((log_data_ptr+ret) < (sizeof(log_data)-4))) {
		memcpy(log_data+log_data_ptr, print_buf, ret);
		log_data_ptr += ret;
		sprintf(log_data+log_data_ptr, "\r\n");
		log_data_ptr += 2;
	}
}
