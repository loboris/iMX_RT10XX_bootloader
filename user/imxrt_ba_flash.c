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
#include "imxrt_ba_flash.h"
#include "fsl_debug_console.h"

// Check if sector is already erased
//----------------------------------
int _sector_erased(uint32_t address)
{
	DCACHE_CleanInvalidateByRange(address, SECTOR_SIZE);
	uint8_t *psect = (uint8_t *)address;
	int i = 0;
	for (i=0; i<SECTOR_SIZE; i++) {
		if (psect[i] != 0xFF) break;
	}
	return i;
}
 
// Check if flash data are already equal to provided buffer data (to be programmed)
//----------------------------------------------------------------------
int _check_flash_data(uint32_t address, uint8_t * data, uint32_t length)
{
	DCACHE_CleanInvalidateByRange(address, length);
	uint8_t *pflash = (uint8_t *)address;

	int i = 0;
	for (i=0; i<length; i++) {
		if (pflash[i] != data[i]) break;
	}
	return i;
}
 
// Erase flash sectors (4096 bytes) at 'address'
//-----------------------------------------------------
status_t flash_erase(uint32_t address, uint32_t length)
{
	status_t status = kStatus_Success;

	// sectors to earse
	uint32_t sectors = 0;

	// Check if the parameters are valid
	if (0 != (address % (uint32_t)SECTOR_SIZE)) return FERR_ADDRESS_ALIGN;
	if (address < BOOT_BACKUP_RECORD_ADDRESS) return FERR_ADDRESS_MIN;
	if ((address+length - BOOTLOADER_FLEXSPI_AMBA_BASE) > FLASH_MAX_LENGTH) return FERR_ADDRESS_MAX;

	// calulate the number of sectors to erase
	sectors = length / (uint32_t)SECTOR_SIZE;
	if  (0 != (length % (uint32_t)SECTOR_SIZE)) sectors += 1;

	for (uint32_t i = 0; i < sectors; i++) {
		if (_sector_erased(address) < SECTOR_SIZE) {
			status = flexspi_nor_flash_erase_sector(BOOTLOADER_FLEXSPI, address-BOOTLOADER_FLEXSPI_AMBA_BASE);
			if (kStatus_Success != status) return FERR_ERASE;
			if (_sector_erased(address) < SECTOR_SIZE) return FERR_ERASE;
			address += SECTOR_SIZE;
		}
	}

	return FERR_OK;
}

// Program flash page (256 bytes) at 'address' from buffer 'data'
// It is assumed that the page sector was previously erased
//--------------------------------------------------------
status_t flash_program_page(uint32_t address, void * data)
{
	status_t status;

	if (_check_flash_data(address, (uint8_t *)data, FLASH_PAGE_SIZE) < FLASH_PAGE_SIZE) {
		status = flexspi_nor_flash_page_program(BOOTLOADER_FLEXSPI, address-BOOTLOADER_FLEXSPI_AMBA_BASE, (void *)data);
		if (kStatus_Success != status) return FERR_PROGRAM_PAGE;
	}

	return FERR_OK;
}

// Program 'length' bytes to flash at 'address' from buffer 'data'
//-----------------------------------------------------------------------------
status_t flash_program_buffer(uint32_t address, uint8_t *data, uint32_t length)
{
	status_t status;

	uint32_t sect_addr = address % SECTOR_SIZE;
	if ((sect_addr+length) > SECTOR_SIZE) return FERR_LENGTH;
	if ((address % (uint32_t)FLASH_PAGE_SIZE)) return FERR_ADDRESS_ALIGN;

	// check if the same data is already programmed
	if (_check_flash_data(address, data, length) == length) return FERR_OK;

	if (sect_addr == 0) {
		// program at beginning of the sector, erase first
		status = flash_erase(address, SECTOR_SIZE);
		if (kStatus_Success != status) return FERR_ERASE;
	}

	// Program page by page
	while (length > 0) {
		uint32_t prog_len = (length > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : length;
		status = flexspi_nor_flash_buffer_program(BOOTLOADER_FLEXSPI, address-BOOTLOADER_FLEXSPI_AMBA_BASE, (const uint32_t *)data, prog_len);
		if (kStatus_Success != status)	return FERR_PROGRAM_BUFFER;
		length -= prog_len;
		address += FLASH_PAGE_SIZE;
		data += FLASH_PAGE_SIZE;
	}

	return FERR_OK;
}

// Read 'length' bytes from flash at 'address' to buffer 'data'
//------------------------------------------------------------------
void flash_read(uint32_t address, void * data,const uint32_t length)
{
	DCACHE_CleanInvalidateByRange(address, length);
	memcpy(data, (void *)(address), length);
}
