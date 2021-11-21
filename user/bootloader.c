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

#include "fsl_flexspi.h"
#include "app.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_common.h"
#include "virtual_com.h"
#include "imxrt_ba_monitor.h"
#include "board_drive_led.h"
#include "imxrt_ba_flash.h"
#include "imxrt_ba_cdc.h"
#include "fsl_dcp.h"

AT_NONCACHEABLE_SECTION(volatile boot_rec_t boot_rec);
extern void JumpToApp(uint32_t address);
uint32_t CPUFreq = 600000000;

static dcp_config_t dcpConfig;
AT_NONCACHEABLE_SECTION(static unsigned char outputSha256[SHA_HASH_SIZE]);
unsigned char *sha256_hash = outputSha256;
app_rec_t app_record;


//------------------------------------------------------------
void call_application(uint32_t address, uint32_t reset_handle)
{
	__disable_irq();
	/*
	// Disable all enabled interrupts in NVIC.
	memset((uint32_t *)NVIC->ICER, 0xFF, sizeof(NVIC->ICER));
	// Clear all pending interrupt requests in NVIC.
	memset((uint32_t *)NVIC->ICPR, 0xFF, sizeof(NVIC->ICPR));
	// Disable SysTick and clear its exception pending bit.
	//SysTick->CTRL = 0;
	//SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
	*/
	uint32_t vector = address + 0x2000; 
	// write_vtor
	unsigned long *pVTOR = (unsigned long*)0xE000ED08;
  *pVTOR = vector;
	
	// Rebase the Stack Pointe
  __set_MSP(*(uint32_t *) vector);
	
	// SystemInit();
	__enable_irq();
	
	// Load the Reset Handler address of the application
	// jump to reset handle address
  uint32_t  app_start_address = vector + reset_handle;

  // === Jump to application Reset Handler in the application ===
  JumpToApp(app_start_address);
}

//-------------------------------------
static inline void ticks_cpu_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

//------------------------
void delay_ms(uint32_t ms)
{
	uint32_t tmo = CPUFreq * ms; 
	DWT->CYCCNT = 0;
	while (DWT->CYCCNT < tmo) {
		;
	}
}

//---------------------------------------------
static void LED_blink(uint32_t n, uint32_t dur)
{
	for (uint32_t i=0; i<n; i++) {
		LED_on();
		delay_ms(dur);
		LED_off();
		delay_ms(dur);
	}
}

//------------------------------------------------
bool app_sha256(uint32_t address, uint32_t length)
{
	dcp_handle_t m_handle;

	m_handle.channel    = kDCP_Channel0;
	m_handle.keySlot    = kDCP_KeySlot0;
	m_handle.swapConfig = kDCP_NoSwap;

	size_t outLength = SHA_HASH_SIZE;
	memset(sha256_hash, 0, outLength);

	const uint8_t *message = (const uint8_t *)address;

	// Calulate SHA-256
	DCACHE_CleanInvalidateByRange(address, length);
	status_t status = DCP_HASH(DCP, &m_handle, kDCP_Sha256, message, length, outputSha256, &outLength);

	return ((kStatus_Success != status) || (outLength != 32u));
}

// Read boot record from main or backup sector
// into 'boot_rec' variable an check it
//----------------------------
int checkBootRecord(bool main)
{
	uint32_t crc;
	uint32_t addr = ((main) ? BOOT_RECORD_ADDRESS : BOOT_BACKUP_RECORD_ADDRESS);

	memset((void *)&boot_rec, 0, sizeof(boot_rec_t));
	flash_read(addr, (void *)&boot_rec, sizeof(boot_rec_t));

	if (memcmp((void *)boot_rec.ID, BOOT_RECORD_ID, sizeof(boot_rec.ID))) {
		log_print("Error: %s boot sect size", (main) ? "main" : "backup");
		return -1;
	}

	crc = crc32((const void *)&boot_rec, sizeof(boot_rec_t)-sizeof(uint32_t), 0);
	if (boot_rec.crc != crc) {
		log_print("Error: %s boot sect CRC", (main) ? "main" : "backup");
		return -2;
	}
	return 0;
}

// Initialize 'boot_rec' variable to valid structure
//--------------------------
static void initBootRecord()
{
	memset((void *)&boot_rec, 0, sizeof(boot_rec_t));
	memcpy((void *)boot_rec.ID, BOOT_RECORD_ID, 16);
	uint32_t crc = crc32((const void *)&boot_rec, sizeof(boot_rec_t)-sizeof(uint32_t), 0);
	boot_rec.crc = crc;
}

// Program 'boot_rec' variable to main or backup boot sector
//-----------------------------
bool writeBootRecord(bool main)
{
	uint8_t bootrec_buf[FLASH_PAGE_SIZE];
	status_t status;

	memset(bootrec_buf, 0xFF, FLASH_PAGE_SIZE);
	memcpy(bootrec_buf, (void *)&boot_rec, sizeof(boot_rec_t));
	uint32_t addr = ((main) ? BOOT_RECORD_ADDRESS : BOOT_BACKUP_RECORD_ADDRESS);

	status = flash_erase(addr, SECTOR_SIZE);
	if (status != kStatus_Success) {
		log_print("Error: erase %s boot sect", (main) ? "main" : "backup");
		return false;
	}

	status = flash_program_page(addr, (void *)bootrec_buf);
	if (status != kStatus_Success) {
		log_print("Error: program %s boot sect", (main) ? "main" : "backup");
		return false;
	}
	return true;
}

// Application's boot record is in 'app_record'
// Try to start it
//-----------------------------
static bool try_start_app(void)
{
	uint32_t size = app_record.size & 0x00FFFFFF;
	if ((size >= MIN_APP_SIZE) && (size <= MAX_APP_SIZE)) {
		// check application's SHA256
		app_sha256(app_record.address, app_record.size & 0x00FFFFFF);
		if (memcmp(app_record.sha256, sha256_hash, SHA_HASH_SIZE) == 0) {
			uint32_t app_address = app_record.address + 0x2004;
			uint32_t reset_handle = (*((volatile uint32_t *) app_address)) - app_record.address - 0x2000;
			// Start the application, does not return
			call_application(app_record.address, reset_handle);
		}
		else log_print("Start app: wrong CRC");;
	}
	else log_print("Start app: wrong size");
	return false;
}

// Check for valid boot application and start it if found
//----------------------
void check_application()
{
	// Check if user button was pressed on start
	uint32_t user_button = GPIO_PinRead(BOARD_USER_BUTTON_GPIO, BOARD_USER_BUTTON_GPIO_PIN);

	// Check if bootloader data area (in Flash at 0xF000) contains the bootloader info structure
	// If none was found, the new one will be initialized

	int res = -3;
	while (res < 0) {
		// Check the main boot record
		res = checkBootRecord(true);
		if (res < 0) {
			// === main boot record does not exist or is corrupted, check backup ===
			log_print("No main boot rec");
			res = checkBootRecord(false);
			if (res >= 0) {
				// backup record ok, copy to main
				log_print("backup->main");
				writeBootRecord(true);
				res = -3;
			}
			else {
				// initialize and write both boot records
				log_print("Boot records init");
				initBootRecord();
				writeBootRecord(true);
				writeBootRecord(false);
				// indicate boot record initialization
				LED_blink(1, 1000);
				res = -3;
			}
		}
		else {
			// === main boot record OK, check the backup one ===
			int crcmain = res;
			res = checkBootRecord(false);
			if (res != crcmain) {
				// copy main record to the backup one
				log_print("main->backup");
				writeBootRecord(false);
				res = -3;
			}
			else {
				res = checkBootRecord(true);
			}
		}
		if (res < 0) LED_blink(1, 500);
	}

	// Boot records OK, check if user button was pressed
	if (user_button == 0) {
		// user button pressed, stay in bootloader mode
		log_print("User button pressed");
		LED_blink(5, 200);
		return;
	}

	// User button NOT pressed, continue to application
	// Try to start application marked as active
	memcpy(&app_record, (void *)boot_rec.apps[0].name, sizeof(app_rec_t));
	if (app_record.size & APP_FLAG_ACTIVE) {
		try_start_app();
		log_print("Active app0 not started");
	}
	memcpy(&app_record, (void *)boot_rec.apps[1].name, sizeof(app_rec_t));
	if (app_record.size & APP_FLAG_ACTIVE) {
		try_start_app();
		log_print("Active app1 not started");
	}

	// no application is marked as active, try to start the one which has the size set
	memcpy(&app_record, (void *)boot_rec.apps[0].name, sizeof(app_rec_t));
	uint32_t size = app_record.size & 0x00FFFFFF;
	if ((size >= MIN_APP_SIZE) && (size <= MAX_APP_SIZE)) {
		try_start_app();
		log_print("App0 not started");
	}
	else log_print("App0 not configured");
	memcpy(&app_record, (void *)boot_rec.apps[1].name, sizeof(app_rec_t));
	size = app_record.size & 0x00FFFFFF;
	if ((size >= MIN_APP_SIZE) && (size <= MAX_APP_SIZE)) {
		try_start_app();
		log_print("App1 not started");
	}
	else log_print("App1 not configured");

	log_print("User button NOT pressed");
}

//============
int main(void)
{
	BOARD_ConfigMPU();
	BOARD_InitPins();
	BOARD_BootClockRUN();
	//BOARD_InitDebugConsole();
	SCB_DisableDCache();
	flexspi_nor_flash_init(BOOTLOADER_FLEXSPI);
	// flexspi_nor_enable_quad_mode(BOOTLOADER_FLEXSPI);
	LED_init();
	CPUFreq = CLOCK_GetFreq(kCLOCK_CpuClk) / 1000;
	ticks_cpu_init();

	// Initialize DCP
	DCP_GetDefaultConfig(&dcpConfig);
	#if DCP_TEST_USE_OTP_KEY
	// Set OTP key type in IOMUX registers before initializing DCP.
	// Software reset of DCP must be issued after changing the OTP key type.
	DCP_OTPKeySelect(kDCP_OTPMKKeyLow);
	#endif
	// Reset and initialize DCP
	DCP_Init(DCP, &dcpConfig);

	// Init user button
	#ifdef BOARD_USER_BUTTON_PIN
	IOMUXC_SetPinMux(BOARD_USER_BUTTON_PIN, 1U);
  uint32_t pad_config = 0U;
  pad_config |= IOMUXC_SW_PAD_CTL_PAD_SPEED(0x01);  // medium(100MHz)
	pad_config |= IOMUXC_SW_PAD_CTL_PAD_PKE(1) /* Pull/Keeper Enabled*/ |	IOMUXC_SW_PAD_CTL_PAD_PUE(1) /* Pull selected*/ |	IOMUXC_SW_PAD_CTL_PAD_PUS(3);
	pad_config |= IOMUXC_SW_PAD_CTL_PAD_DSE(0x0000) /* output driver disabled*/ |	IOMUXC_SW_PAD_CTL_PAD_HYS(1U) /* Hysteresis enabled*/;
	IOMUXC_SetPinConfig(BOARD_USER_BUTTON_PIN, pad_config);
	#endif
	gpio_pin_config_t button_config = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};
	GPIO_PinInit(BOARD_USER_BUTTON_GPIO, BOARD_USER_BUTTON_GPIO_PIN, &button_config);

	check_application();

	// ------------------------------------------------------
	// We will come here if no valid application was detected
	// or the bootloader button was pressed
	// ------------------------------------------------------
	vcom_cdc_init();
	 
	while (1)	{
		imxrt_ba_monitor_run();
	}
}
