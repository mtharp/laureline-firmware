/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "bootloader.h"
#include "ff.h"
#include "init.h"
#include "linker.h"
#include "stm32/mmc.h"
#include "stm32/serial.h"
#include "stm32/spi.h"

#define MAIN_STACK 512
OS_STK main_stack[MAIN_STACK];
OS_TID main_tid;

FATFS MMC_FS;
#define MMC_FIRMWARE_FILENAME "ll.hex"


static void
jump_application(void) {
	typedef void (*func_t)(void);
	uint32_t *vtor = _user_start;
	func_t func = (func_t)vtor[1];
	int i;

	serial_printf(&Serial1, "Starting application at %08x, entry point is %08x\r\n", &_user_start, func);
	CoTickDelay(MS2ST(100));
	DISABLE_IRQ();
	/* Clear and disable interrupt vectors */
	SCB->ICSR = SCB_ICSR_PENDSVCLR | SCB_ICSR_PENDSTCLR;
	for (i = 0; i < 8; i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}
	ENABLE_IRQ();
	SCB->VTOR = (uint32_t)vtor;
	__set_MSP((uint32_t)vtor[0]);
	func();
}


void
main_thread(void *pdata) {
	int16_t rc;
	FIL fp;
	UINT nread;
	const char *errmsg;
	static uint8_t buf[512];

	SPI3_Dev.cs_pad = SDIO_CS_PAD;
	SPI3_Dev.cs_pin = SDIO_CS_PNUM;
	spi_start(&SPI3_Dev, 0);
	mmc_start();
	GPIO_OFF(SDIO_PWR);
	serial_printf(&Serial1, "Allowed region: %08x - %08x\r\n", _user_start, _user_end);
	serial_puts(&Serial1, "\r\nstarting\r\n");
	while (1) {
		CoTickDelay(MS2ST(250));
		rc = mmc_connect();
		if (rc == EERR_OK) {
			serial_puts(&Serial1, "SD connected\r\n");
		} else if (rc == EERR_TIMEOUT) {
			serial_puts(&Serial1, "Timed out waiting for SD\r\n");
			continue;
		} else {
			serial_puts(&Serial1, "Failed to connect to SD\r\n");
			continue;
		}
		jump_application();

		serial_puts(&Serial1, "Mounting SD filesystem\r\n");
		if (f_mount(0, &MMC_FS) != FR_OK) {
			serial_puts(&Serial1, "ERROR: Unable to mount filesystem\r\n");
			continue;
		}

		serial_puts(&Serial1, "Opening file " MMC_FIRMWARE_FILENAME "\r\n");
		if (f_open(&fp, MMC_FIRMWARE_FILENAME, FA_READ) != FR_OK) {
			serial_puts(&Serial1, "Error opening file, maybe it does not exist\r\n");
			continue;
		}

		bootloader_start();
		while (bootloader_status == BLS_FLASHING) {
			if (f_read(&fp, buf, sizeof(buf), &nread) != FR_OK) {
				serial_puts(&Serial1, "Error reading file\r\n");
				break;
			}
			if (nread == 0) {
				serial_puts(&Serial1, "Error: premature end of file\r\n");
				break;
			}
			errmsg = bootloader_feed(buf, nread);
			if (errmsg != NULL) {
				serial_printf(&Serial1, "Error flashing firmware: %s\r\n", errmsg);
				break;
			}
		}

		if (bootloader_status == BLS_DONE) {
			serial_puts(&Serial1, "New firmware successfully loaded\r\n");
			CoTickDelay(MS2ST(100));
			jump_application();
		} else {
			serial_puts(&Serial1, "ERROR: Reset to try again or load last known good firmware\r\n");
			//HALT();
		}
		HALT();
		CoTickDelay(S2ST(1));
	}
}


void
main(void) {
	setup_clocks(ONBOARD_CLOCK);
	CoInitOS();
	serial_start(&Serial1, 115200);
	main_tid = CoCreateTask(main_thread, NULL, THREAD_PRIO_MAIN,
			&main_stack[MAIN_STACK-1], MAIN_STACK, "main");
	ASSERT(main_tid != E_CREATE_FAIL);
	CoStartOS();
	while (1) {}
}
