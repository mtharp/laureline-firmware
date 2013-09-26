/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "init.h"
#include "periph/mmc.h"
#include "periph/serial.h"
#include "periph/spi.h"

#define MAIN_STACK 512
OS_STK main_stack[MAIN_STACK];
OS_TID main_tid;


void
main_thread(void *pdata) {
	int16_t rc;
	static uint8_t buf[512];
	int i, j;
	SPI3_Dev.cs_pad = SDIO_CS_PAD;
	SPI3_Dev.cs_pin = SDIO_CS_PNUM;
	spi_start(&SPI3_Dev, 0);
	mmc_start();
	serial_puts(&Serial1, "\r\nstarting\r\n");
	while (1) {
		CoTickDelay(MS2ST(250));
		mmc_disconnect();
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
		//CoTickDelay(MS2ST(100));
		if (mmc_start_read(0)) {
			serial_puts(&Serial1, "mmc_start_read failed\r\n");
			continue;
		} else {
			serial_puts(&Serial1, "mmc_start_read OK\r\n");
		}
		//CoTickDelay(MS2ST(100));
		if (mmc_read_sector(buf)) {
			serial_puts(&Serial1, "mmc_read_sector failed\r\n");
			continue;
		} else {
			serial_puts(&Serial1, "mmc_read_sector OK\r\n");
		}
		//CoTickDelay(MS2ST(100));
		if (mmc_stop_read()) {
			serial_puts(&Serial1, "mmc_stop_read failed\r\n");
			continue;
		} else {
			serial_puts(&Serial1, "mmc_stop_read OK\r\n");
		}
		for (i = 0; i < 512; i += 16) {
			serial_printf(&Serial1, "%04x  ", i);
			for (j = i; j < i + 8; j++) {
				serial_printf(&Serial1, "%02x ", buf[j]);
			}
			serial_puts(&Serial1, " ");
			for (j = i + 8; j < i + 16; j++) {
				serial_printf(&Serial1, "%02x ", buf[j]);
			}
			serial_puts(&Serial1, "\r\n");
		}
		serial_puts(&Serial1, "\r\n\r\n");
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
