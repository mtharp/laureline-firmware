/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "ff.h"
#include "init.h"
#include "stm32/mmc.h"
#include "stm32/serial.h"
#include "stm32/spi.h"

#define MAIN_STACK 512
OS_STK main_stack[MAIN_STACK];
OS_TID main_tid;

FATFS MMC_FS;
#define MMC_FIRMWARE_FILENAME "ll.hex"


void
main_thread(void *pdata) {
	int16_t rc;
	int i = 0;
	FIL fp;
	SPI3_Dev.cs_pad = SDIO_CS_PAD;
	SPI3_Dev.cs_pin = SDIO_CS_PNUM;
	spi_start(&SPI3_Dev, 0);
	mmc_start();
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
		serial_printf(&Serial1, "Done! %d\r\n", ++i);
		CoTickDelay(S2ST(1));
	}
}


void
main(void) {
	CoInitOS();
	setup_clocks(ONBOARD_CLOCK);
	serial_start(&Serial1, 115200);
	main_tid = CoCreateTask(main_thread, NULL, THREAD_PRIO_MAIN,
			&main_stack[MAIN_STACK-1], MAIN_STACK, "main");
	ASSERT(main_tid != E_CREATE_FAIL);
	CoStartOS();
	while (1) {}
}
