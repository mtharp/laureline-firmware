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
#include "init.h"
#include "serial.h"

#include <string.h>

#define TASK0_PRI 10
#define TASK0_STACK 512
OS_STK task0stack[TASK0_STACK];
OS_TID task0id;


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
task0(void *pdata) {
	uint8_t data, err;
	uint32_t flags;
	cli_banner();
	while (1) {
		flags = CoWaitForMultipleFlags(0
				| 1 << Serial1.rx_flag
				// | 1 << Serial4.rx_flag,
				, OPT_WAIT_ANY, S2ST(1), &err);
		if (err != E_OK && err != E_TIMEOUT) {
			HALT();
		}
		if (flags & (1 << Serial1.rx_flag)) {
			data = Serial1.rx_char;
			cli_feed(data);
		}
		if (flags & (1 << Serial4.rx_flag)) {
			serial_puts(&Serial1, "got gps\r\n");
		}
	}
}


void
main(void) {
	CoInitOS();
	setup_clocks(ONBOARD_CLOCK);
	serial_start(&Serial1, USART1, 115200);
	serial_start(&Serial4, UART4, 57600);
	cli_set_output(&Serial1);
	load_eeprom();
	task0id = CoCreateTask(task0, NULL, TASK0_PRI,
			&task0stack[TASK0_STACK-1], TASK0_STACK);
	CoStartOS();
	while (1) {}
}
