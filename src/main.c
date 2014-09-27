/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "task.h"

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

#include <string.h>

TaskHandle_t thread_main;
serial_t *const cli_serial = &Serial1;
serial_t *gps_serial;
uint8_t watchdog_main, watchdog_net;

static int did_watchdog;

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
    log_write(LOG_NOTICE, "kernel", BOARD_NAME " version " VERSION " started");
    if (did_watchdog) {
        log_write(LOG_CRIT, "kernel", "Device was previously reset by watchdog timer!");
    }
}


static void
main_thread(void *pdata) {
    QueueSetHandle_t qs;
    QueueSetMemberHandle_t active;
    int16_t val;

    ASSERT((qs = xQueueCreateSet(SERIAL_RX_SIZE * 3)));
    serial_start(cli_serial, 115200, qs);
    serial_start(&Serial4, 57600, qs);
    serial_start(&Serial5, cfg.gps_baud_rate ? cfg.gps_baud_rate : 57600, qs);
    cli_set_output(cli_serial);
    log_start(cli_serial);
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
        ublox_configure();
        if (HAS_FEATURE(PPSEN) && (cfg.flags & FLAG_PPSEN)) {
            GPIO_OFF(PPSEN);
        }
    }
    cl_enabled = 0;
    while (1) {
        watchdog_main = 5;
        active = xQueueSelectFromSet(qs, pdMS_TO_TICKS(1000));
        if (active == cli_serial->rx_q) {
            val = serial_get(cli_serial, TIMEOUT_NOBLOCK);
            ASSERT(val >= 0);
            cli_feed(val);
        } else if (active == gps_serial->rx_q) {
            val = serial_get(gps_serial, TIMEOUT_NOBLOCK);
            ASSERT(val >= 0);
            gps_byte_received(val);
            if (cfg.flags & FLAG_GPSOUT) {
                char tmp = val;
                serial_write(&Serial5, &tmp, 1);
            }
        }
    }
}


void
main(void) {
    GPIO_OFF(E_NRST);
    unstick_i2c();
    GPIO_ON(E_NRST);
    setup_clocks((int)info_get(boot_table, INFO_HSE_FREQ));
    iwdg_start(4, 0xFFF);
    watchdog_main = watchdog_net = 5;
    ASSERT(xTaskCreate(main_thread, "main", MAIN_STACK_SIZE, NULL,
                THREAD_PRIO_MAIN, &thread_main));
    vTaskStartScheduler();
}
