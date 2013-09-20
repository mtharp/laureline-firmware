/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "cmdline.h"
#include "eeprom.h"
#include "gps/parser.h"
#include "gps/ublox.h"
#include "init.h"
#include "ppscapture.h"
#include "serial.h"
#include "tcpip.h"
#include "vtimer.h"

#include <string.h>

#define MAIN_STACK 512
OS_STK main_stack[MAIN_STACK];
OS_TID main_tid;


static void
load_eeprom(void) {
	int16_t rc;
	rc = eeprom_read_cfg();
	if (rc == EERR_UPGRADE
			|| (rc == EERR_OK && cfg.version == 0)) {
		memset(&cfg_bytes[12], 0, EEPROM_CFG_SIZE - 12);
		cfg.version = CFG_VERSION;
		rc = eeprom_write_cfg();
	} else if (rc != EERR_OK) {
		memset(cfg_bytes, 0, EEPROM_CFG_SIZE);
		cfg.version = CFG_VERSION;
		rc = eeprom_write_cfg();
	}
}

void
main_thread(void *pdata) {
	uint8_t err;
	uint32_t flags;
	load_eeprom();
	tcpip_start();
	cli_banner();
	ublox_configure(&Serial4);
	while (1) {
		flags = CoWaitForMultipleFlags(0
				| 1 << Serial1.rx_flag
				| 1 << Serial4.rx_flag
				, OPT_WAIT_ANY, S2ST(1), &err);
		if (err != E_OK && err != E_TIMEOUT) {
			HALT();
		}
		if (flags & (1 << Serial1.rx_flag)) {
			cli_feed(Serial1.rx_char);
		}
		if (flags & (1 << Serial4.rx_flag)) {
			gps_byte_received(Serial4.rx_char);
		}
	}
}


void
main(void) {
	CoInitOS();
	setup_clocks(ONBOARD_CLOCK);
	serial_start(&Serial1, 115200);
	serial_start(&Serial4, 57600);
	cli_set_output(&Serial1);
	ppscapture_start();
	vtimer_start();
	main_tid = CoCreateTask(main_thread, NULL, THREAD_PRIO_MAIN,
			&main_stack[MAIN_STACK-1], MAIN_STACK);
	ASSERT(main_tid != E_CREATE_FAIL);
	CoStartOS();
	while (1) {}
}
