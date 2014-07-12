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
#include "info_table.h"
#include "init.h"
#include "logging.h"
#include "ppscapture.h"
#include "net/tcpip.h"
#include "version.h"
#include "vtimer.h"
#include "stm32/eth_mac.h"
#include "stm32/iwdg.h"
#include "stm32/serial.h"
#include "util/queue.h"

#include <string.h>

#define MAIN_STACK 512
OS_STK main_stack[MAIN_STACK];
OS_TID main_tid;
serial_t *const cli_serial = &Serial1;
serial_t *gps_serial;
static int did_watchdog;

uint32_t __attribute__((section(".uninit"))) entering_standby;
#define ENTERING_STANDBY 0x5d6347c2

/* info table for the boot stub */
const info_entry_t boot_table[] = {
	{INFO_HWVER, (void*)HW_VERSION},
	{INFO_HSE_FREQ, (void*)HSE_FREQ},
	{INFO_END, NULL},
};

/* info table for the application itself */
const info_entry_t app_table[] = {
	{INFO_APPVER, VERSION},
	{INFO_END, NULL},
};


static void
load_eeprom(void) {
	int16_t rc;
	rc = eeprom_read_cfg();
	if (rc != EERR_OK) {
		memset(&cfg, 0, sizeof(cfg));
		cfg.version = CFG_VERSION;
		cli_puts("ERROR: EEPROM is invalid, run 'save' or 'defaults' to clear\r\n");
	}
}


static void
test_reset(void) {
	uint32_t sr = RCC->CSR;
	RCC->CSR = RCC_CSR_RMVF;
	if (sr & (RCC_CSR_WWDGRSTF | RCC_CSR_IWDGRSTF)) {
		cli_puts("\r\n\r\nERROR: Device was reset by watchdog timer!\r\n");
		did_watchdog = 1;
	}
}


void
log_startup(void) {
	log_write(LOG_NOTICE, "kernel", "GPS NTP Server version " VERSION " started");
	if (did_watchdog) {
		log_write(LOG_CRIT, "kernel", "Device was previously reset by watchdog timer!");
	}
}


void
enter_standby(void) {
	/* Shutdown onboard peripherals */
	mac_stop();
	if (!(cfg.flags & FLAG_GPSEXT)) {
		ublox_stop(gps_serial);
	}
	serial_drain(cli_serial);
	serial_drain(gps_serial);
	GPIO_ON(SDIO_PDOWN);
	/* Trigger a reset in order to disable the watchdog timer, otherwise it
	 * will just wake us up after a few seconds.
	 */
	entering_standby = ENTERING_STANDBY;
	NVIC_SystemReset();
	while (1) {}
}


static void
finish_standby(void) {
	entering_standby = 0;
	RCC->CSR = RCC_CSR_RMVF;
	PWR->CR |= PWR_CR_PDDS | PWR_CR_LPDS;
	PWR->CSR = PWR_CSR_WUF | PWR_CSR_SBF;
	SCB->SCR |= SCB_SCR_SLEEPDEEP;
	__WFE();
}


static void
main_thread(void *pdata) {
	uint8_t err;
	int16_t val;
	uint32_t flags;
	serial_start(cli_serial, 115200);
	serial_start(&Serial4, 57600);
	serial_start(&Serial5, cfg.gps_baud_rate ? cfg.gps_baud_rate : 57600);
	cli_set_output(cli_serial);
	log_start(cli_serial);
	log_sethostname("gps-ntp");
	cl_enabled = 1;

	load_eeprom();
	cfg.flags &= ~FLAG_HOLDOVER_TEST;
	if (cfg.flags & FLAG_GPSEXT) {
		gps_serial = &Serial5;
	} else {
		gps_serial = &Serial4;
	}
	if (!cfg.holdover) {
		cfg.holdover = 60;
	}
	if (!cfg.loopstats_interval) {
		cfg.loopstats_interval = 60;
	}
	ppscapture_start();
	vtimer_start();
	tcpip_start();
	test_reset();
	cli_banner();
	if (!(cfg.flags & FLAG_GPSEXT)) {
		ublox_configure(gps_serial);
		if (HAS_FEATURE(PPSEN) && (cfg.flags & FLAG_PPSEN)) {
			GPIO_OFF(PPSEN);
		}
	}
	cl_enabled = 0;
	while (1) {
		flags = CoWaitForMultipleFlags(0
				| 1 << cli_serial->rx_q.flag
				| 1 << gps_serial->rx_q.flag
				, OPT_WAIT_ANY, S2ST(1), &err);
		if (err != E_OK && err != E_TIMEOUT) { HALT();
		}
		if (flags & (1 << cli_serial->rx_q.flag)) {
			while ((val = serial_get(cli_serial, TIMEOUT_NOBLOCK)) >= 0) {
				cli_feed(val);
			}
		}
		if (flags & (1 << gps_serial->rx_q.flag)) {
			while ((val = serial_get(gps_serial, TIMEOUT_NOBLOCK)) >= 0) {
				gps_byte_received(val);
				if (cfg.flags & FLAG_GPSOUT) {
					char tmp = val;
					serial_write(&Serial5, &tmp, 1);
				}
			}
		}
	}
}


void
main(void) {
	if (entering_standby == ENTERING_STANDBY) {
		finish_standby();
	}
	GPIO_OFF(E_NRST);
	unstick_i2c();
	GPIO_ON(E_NRST);
	setup_clocks((int)info_get(boot_table, INFO_HSE_FREQ));
	iwdg_start(4, 0xFFF);
	CoInitOS();
	main_tid = CoCreateTask(main_thread, NULL, THREAD_PRIO_MAIN,
			&main_stack[MAIN_STACK-1], MAIN_STACK, "main");
	ASSERT(main_tid != E_CREATE_FAIL);
	CoStartOS();
	while (1) {}
}
