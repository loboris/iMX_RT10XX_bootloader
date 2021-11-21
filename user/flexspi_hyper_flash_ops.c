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

/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flexspi.h"
#include "app.h"
#include "fsl_debug_console.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define NOR_CMD_LUT_SEQ_IDX_READ_NORMAL 0
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST 1
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD 2
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS 3
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE 4
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR 5
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_SINGLE 6
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD 7
#define NOR_CMD_LUT_SEQ_IDX_READID 8
#define NOR_CMD_LUT_SEQ_IDX_WRITESTATUSREG 9
#define NOR_CMD_LUT_SEQ_IDX_ENTERQPI 10
#define NOR_CMD_LUT_SEQ_IDX_EXITQPI 11
#define NOR_CMD_LUT_SEQ_IDX_READSTATUSREG 12
#define NOR_CMD_LUT_SEQ_IDX_ERASECHIP 13
#define CUSTOM_LUT_LENGTH 60
#define FLASH_BUSY_STATUS_POL 1
#define FLASH_BUSY_STATUS_OFFSET 0

//-------------------------------------------
static flexspi_device_config_t deviceconfig =
{
    .flexspiRootClk = 40000000,
    .flashSize = FLASH_SIZE,
    .CSIntervalUnit = kFLEXSPI_CsIntervalUnit1SckCycle,
    .CSInterval = 2,
    .CSHoldTime = 3,
    .CSSetupTime = 3,
    .dataValidTime = 0,
    .columnspace = 0,
    .enableWordAddress = 0,
    .AWRSeqIndex = 0,
    .AWRSeqNumber = 0,
    .ARDSeqIndex = NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD,
    .ARDSeqNumber = 1,
    .AHBWriteWaitUnit = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
    .AHBWriteWaitInterval = 0,
};

//--------------------------------------------
static uint32_t customLUT[CUSTOM_LUT_LENGTH] =
{
    // Normal read mode -SDR
    [4 * NOR_CMD_LUT_SEQ_IDX_READ_NORMAL] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x03, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    [4 * NOR_CMD_LUT_SEQ_IDX_READ_NORMAL + 1] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    // Fast read mode - SDR
    [4 * NOR_CMD_LUT_SEQ_IDX_READ_FAST] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x0B, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    [4 * NOR_CMD_LUT_SEQ_IDX_READ_FAST + 1] = FLEXSPI_LUT_SEQ(
        kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x08, kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x04),

    // Fast read quad mode - SDR
    [4 * NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x6B, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    [4 * NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD + 1] = FLEXSPI_LUT_SEQ(
        kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD, 0x08, kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x04),

    // Read extend parameters
    [4 * NOR_CMD_LUT_SEQ_IDX_READSTATUS] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x05, kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x04),

    // Write Enable
    [4 * NOR_CMD_LUT_SEQ_IDX_WRITEENABLE] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x06, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    // Erase Sector 
    [4 * NOR_CMD_LUT_SEQ_IDX_ERASESECTOR] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x20, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),

    // Page Program - single mode
    [4 * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_SINGLE] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x02, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    [4 * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_SINGLE + 1] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    // Page Program - quad mode
    [4 * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x32, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    [4 * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD + 1] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    // Read ID
    [4 * NOR_CMD_LUT_SEQ_IDX_READID] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xAB, kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x18),
    [4 * NOR_CMD_LUT_SEQ_IDX_READID + 1] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    // Enable Quad mode
    [4 * NOR_CMD_LUT_SEQ_IDX_WRITESTATUSREG] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x01, kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x04),

    // Enter QPI mode
    [4 * NOR_CMD_LUT_SEQ_IDX_ENTERQPI] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x38, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    // Exit QPI mode
    [4 * NOR_CMD_LUT_SEQ_IDX_EXITQPI] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_4PAD, 0xFF, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    // Read status register
    [4 * NOR_CMD_LUT_SEQ_IDX_READSTATUSREG] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x05, kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x04),

    // Erase Chip
    [4 * NOR_CMD_LUT_SEQ_IDX_ERASECHIP] =
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xC7, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
};

/*******************************************************************************
 * Variables
 *****************************************************************************/
extern flexspi_device_config_t deviceconfig;

/*******************************************************************************
 * Code
 ******************************************************************************/

//----------------------------------------------------------------------
status_t flexspi_nor_write_enable(FLEXSPI_Type *base, uint32_t baseAddr)
{
	flexspi_transfer_t flashXfer;
	status_t status;

	// Write enable
	flashXfer.deviceAddress = baseAddr;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Command;
	flashXfer.SeqNumber = 1;
	flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_WRITEENABLE;

	status = FLEXSPI_TransferBlocking(base, &flashXfer);
	return status;
}

#if 0
static inline void flexspi_clock_init(void)
{
	const clock_usb_pll_config_t g_ccmConfigUsbPll = {.loopDivider = 0U};

	CLOCK_InitUsb1Pll(&g_ccmConfigUsbPll);
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd0, 26);   /* Set PLL3 PFD0 clock 332MHZ. */
	CLOCK_SetMux(kCLOCK_FlexspiMux, 0x3); /* Choose PLL3 PFD0 clock as flexspi source clock. */
	CLOCK_SetDiv(kCLOCK_FlexspiDiv, 3);   /* flexspi clock 42M. */
}
#endif

//----------------------------------------------------
status_t flexspi_nor_wait_bus_busy(FLEXSPI_Type *base)
{
	// Wait status ready.
	bool isBusy;
	uint32_t readValue;
	status_t status;
	flexspi_transfer_t flashXfer;

	flashXfer.deviceAddress = 0;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Read;
	flashXfer.SeqNumber = 1;
	flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_READSTATUSREG;
	flashXfer.data = &readValue;
	flashXfer.dataSize = 1;

	do {
		status = FLEXSPI_TransferBlocking(base, &flashXfer);
		if (status != kStatus_Success) return status;
		if (FLASH_BUSY_STATUS_POL) {
			if (readValue & (1U << FLASH_BUSY_STATUS_OFFSET)) isBusy = true;
			else isBusy = false;
		}
		else {
			if (readValue & (1U << FLASH_BUSY_STATUS_OFFSET)) isBusy = false;
			else isBusy = true;
		}
	}	while (isBusy);

	return status;
}

//-------------------------------------------------------
status_t flexspi_nor_enable_quad_mode(FLEXSPI_Type *base)
{
	flexspi_transfer_t flashXfer;
	status_t status;
	uint32_t writeValue = 0x40;

	// Write enable
	status = flexspi_nor_write_enable(base, 0);
	if (status != kStatus_Success) return status;

	/* Enable quad mode. */
	flashXfer.deviceAddress = 0;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Write;
	flashXfer.SeqNumber = 1;
	flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_WRITESTATUSREG;
	flashXfer.data = &writeValue;
	flashXfer.dataSize = 1;

	status = FLEXSPI_TransferBlocking(base, &flashXfer);
	if (status != kStatus_Success) return status;

	status = flexspi_nor_wait_bus_busy(base);
	return status;
}

//---------------------------------------------------------------------------
status_t flexspi_nor_flash_erase_sector(FLEXSPI_Type *base, uint32_t address)
{
	status_t status;
	flexspi_transfer_t flashXfer;

	// Write enable
	status = flexspi_nor_write_enable(base, address);
	if (status != kStatus_Success) return status;

	flashXfer.deviceAddress = address;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Command;
	flashXfer.SeqNumber = 1;
	flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_ERASESECTOR;
	status = FLEXSPI_TransferBlocking(base, &flashXfer);

	if (status != kStatus_Success) return status;

	status = flexspi_nor_wait_bus_busy(base);
	FLEXSPI_SoftwareReset(base);
	return status;
}

//-------------------------------------------------------------------------------------------------------------------
status_t flexspi_nor_flash_buffer_program(FLEXSPI_Type *base, uint32_t address, const uint32_t *src, uint32_t length)
{
	status_t status;
	flexspi_transfer_t flashXfer;

	// To make sure external flash be in idle status, added wait for busy before program data
	// for an external flash without RWW(read while write) attribute.
	status = flexspi_nor_wait_bus_busy(base);
	if (kStatus_Success != status) return status;

	// Write enable
	status = flexspi_nor_write_enable(base, address);
	if (status != kStatus_Success) return status;

	// Prepare page program command
	flashXfer.deviceAddress = address;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Write;
	flashXfer.SeqNumber = 1;
	flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD;
	flashXfer.data = (uint32_t *)src;
	flashXfer.dataSize = length;

	status = FLEXSPI_TransferBlocking(base, &flashXfer);
	if (status != kStatus_Success) return status;

	status = flexspi_nor_wait_bus_busy(base);
	FLEXSPI_SoftwareReset(base);
	return status;
}

//------------------------------------------------------------------------------------------------
status_t flexspi_nor_flash_page_program(FLEXSPI_Type *base, uint32_t address, const uint32_t *src)
{
	status_t status;
	status = flexspi_nor_flash_buffer_program(base, address, src, FLASH_PAGE_SIZE);
	return status;
}

//-----------------------------------------------------------------------
status_t flexspi_nor_get_vendor_id(FLEXSPI_Type *base, uint8_t *vendorId)
{
	uint32_t temp;
	flexspi_transfer_t flashXfer;
	flashXfer.deviceAddress = 0;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Read;
	flashXfer.SeqNumber = 1;
	flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_READID;
	flashXfer.data = &temp;
	flashXfer.dataSize = 1;

	status_t status = FLEXSPI_TransferBlocking(base, &flashXfer);

	*vendorId = temp;

	return status;
}

//-------------------------------------------------
status_t flexspi_nor_erase_chip(FLEXSPI_Type *base)
{
	status_t status;
	flexspi_transfer_t flashXfer;

	// Write enable
	status = flexspi_nor_write_enable(base, 0);
	if (status != kStatus_Success) return status;

	flashXfer.deviceAddress = 0;
	flashXfer.port          = kFLEXSPI_PortA1;
	flashXfer.cmdType       = kFLEXSPI_Command;
	flashXfer.SeqNumber     = 1;
	flashXfer.seqIndex      = NOR_CMD_LUT_SEQ_IDX_ERASECHIP;

	status = FLEXSPI_TransferBlocking(base, &flashXfer);
	if (status != kStatus_Success) return status;

	status = flexspi_nor_wait_bus_busy(base);
	return status;
}

//--------------------------------------------
int flexspi_nor_flash_init(FLEXSPI_Type *base)
{
	flexspi_config_t config;
	status_t status;
	uint8_t vendorID = 0;

	const clock_usb_pll_config_t g_ccmConfigUsbPll = {.loopDivider = 0U};

	CLOCK_InitUsb1Pll(&g_ccmConfigUsbPll);
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd0, 26);   /* Set PLL3 PFD0 clock 360MHZ. */
	CLOCK_SetMux(kCLOCK_FlexspiMux, 0x3); /* Choose PLL3 PFD0 clock as flexspi source clock. */
	CLOCK_SetDiv(kCLOCK_FlexspiDiv, 3);   /* flexspi clock 120M. */

	// Get FLEXSPI default settings and configure the flexspi.
	FLEXSPI_GetDefaultConfig(&config);
	// Set AHB buffer size for reading data through AHB bus.
	config.ahbConfig.enableAHBPrefetch = true;
	//config.ahbConfig.enableAHBBufferable  = true;
	//config.ahbConfig.enableReadAddressOpt = true;
	//config.ahbConfig.enableAHBCachable    = true;

	FLEXSPI_Init(base, &config);

	// Configure flash settings according to serial flash feature.
	FLEXSPI_SetFlashConfig(base, &deviceconfig, kFLEXSPI_PortA1);
	// Update LUT table.
	FLEXSPI_UpdateLUT(base, 0, customLUT, CUSTOM_LUT_LENGTH);
	// Do software reset.
	FLEXSPI_SoftwareReset(base);

	status = flexspi_nor_get_vendor_id(FLEXSPI, &vendorID);
	if (status != kStatus_Success) return status;

	status = flexspi_nor_enable_quad_mode(FLEXSPI);
	if (status != kStatus_Success) return status;

	return status;
}

