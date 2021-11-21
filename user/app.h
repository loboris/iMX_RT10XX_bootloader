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

#ifndef _APP_H_
#define _APP_H_

#include "fsl_flexspi.h"
#include "fsl_cache.h"
#include "board.h"

// boot app constants
#define MIN_APP_SIZE									0x10000		// 64KB
#define MAX_APP_SIZE									0x200000	// 2MB

#define APP_FLAG_ACTIVE								0x01000000
#define SHA_HASH_SIZE									32

#define BOOTLOADER_FLEXSPI FLEXSPI
#define BOOTLOADER_FLEXSPI_AMBA_BASE FlexSPI_AMBA_BASE

#ifdef QSPI_FLASH
#define FLASH_SIZE (8*1024u)
#define FLASH_PAGE_SIZE 256
#define SECTOR_SIZE 0x1000

#define BOOTLOADER_FLEXSPI_CLOCK kCLOCK_FlexSpi

#define FLASH_START_ADDRESS 					0x60010000
#define FLASH_MAX_LENGTH 							(0x60800000-FLASH_START_ADDRESS)

#define APP_START_ADDRESS 						0x60010000
#define BOOTLOADER_START_ADDRESS 			0x60000000

// static variables
#define BOOT_RECORD_ADDRESS           (0x6000F000)
#define BOOT_BACKUP_RECORD_ADDRESS    (0x6000E000)
#define BOOT_RECORD_ID								"i.MXRT10XX_boot"

#define FCFB_BLOCK_ID									(0x42464346)
#define IVT_BLOCK_ID1									(0x60011000)
#define IVT_BLOCK_ID2									(0x60012000)
#define FCFB_BLOCK_ID_DATA						(*((volatile uint32_t *)APP_START_ADDRESS))
#define IVT_BLOCK_ID1_DATA						(*((volatile uint32_t *)(APP_START_ADDRESS+0x1014)))
#define IVT_BLOCK_ID2_DATA						(*((volatile uint32_t *)(APP_START_ADDRESS+0x1004)))

#else
#define FLASH_SIZE 0x10000
#define FLASH_PAGE_SIZE 512
#define BOOTLOADER_DATA_AREA 1
#define SECTOR_SIZE 0x40000

#define BOOTLOADER_FLEXSPI_CLOCK kCLOCK_FlexSpi

#define FLASH_START_ADDRESS 0x60080000
#define FLASH_MAX_LENGTH 0x64000000- FLASH_START_ADDRESS

#define APP_START_ADDRESS 0x60080000
#define BOOTLOADER_START_ADDRESS 0x60000000

// static variable
#define BOOT_STATUS_ADDRESS           (0x60040000 + 4*8)
#define BOOT_STATUS_DATA              (*((volatile uint32_t *) BOOT_STATUS_ADDRESS))
#define BOOT_STATUS_MAGIC							(0x424F4F54)
#endif


// extern functions
extern int flexspi_nor_flash_init(FLEXSPI_Type *base);
extern status_t flexspi_nor_enable_quad_mode(FLEXSPI_Type *base);
extern status_t flexspi_nor_flash_erase_sector(FLEXSPI_Type *base, uint32_t address);
extern status_t flexspi_nor_flash_page_program(FLEXSPI_Type *base, uint32_t address, const uint32_t *src);
extern status_t flexspi_nor_flash_buffer_program(FLEXSPI_Type *base, uint32_t address, const uint32_t *src, uint32_t length);
extern status_t flexspi_nor_hyperflash_cfi(FLEXSPI_Type *base);
extern status_t flexspi_nor_flash_erase_chip(FLEXSPI_Type *base);
 
extern void Reset_Handler(void);

// Boot record structures
typedef struct _app_ {
	char     	name[16];		// application name, NULL terminated string
	uint32_t 	address;		// application address in Flash (min addr 0x600100000)
	uint32_t 	size;				// application size, 24-bil; upper 8-bits are flags
	uint32_t	timestamp;  // application timestamp;
	uint8_t 	sha256[32];	// application's SHA-256 hash calculated over 'size' bytes from 'address'
}	app_rec_t;						// size: 60 bytes

typedef struct _boot_rec_ {
	char     	ID[16];	
	app_rec_t	apps[2];
	uint32_t 	crc;
}	boot_rec_t;						// size: 140 bytes

// global variables and functions
extern unsigned char *sha256_hash;
extern volatile boot_rec_t boot_rec;
extern app_rec_t app_record;

bool app_sha256(uint32_t address, uint32_t length);
bool writeBootRecord(bool main);
int checkBootRecord(bool main);
void delay_ms(uint32_t ms);

#endif // _APP_H_
