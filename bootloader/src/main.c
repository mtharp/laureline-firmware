/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "task.h"

#include "bootloader.h"
#include "ff.h"
#include "init.h"
#include "version.h"
#include "stm32/flash.h"
#include "stm32/mmc.h"
#include "stm32/serial.h"
#include "stm32/spi.h"

TaskHandle_t thread_main;
FATFS MMC_FS;
#define MMC_FIRMWARE_FILENAME "ll.hex"

#define JUMP_TOKEN 0xeefc63d2
uint32_t __attribute__((section(".uninit"))) jump_token;
const uint32_t *user_vtor = _user_start;


static void
reset_and_jump(void) {
    jump_token = JUMP_TOKEN;
    NVIC_SystemReset();
    while (1) {}
}


static void
jump_application(void) {
    typedef void (*func_t)(void);
    func_t entry = (func_t)user_vtor[1];
    if (entry != (void*)0xFFFFFFFF) {
        __set_MSP(user_vtor[0]);
        entry();
    }
}


static void
try_flash(void) {
    int16_t rc;
    FIL fp;
    UINT nread;
    const char *errmsg;
    static uint8_t buf[512];

    SPI3_Dev.cs_pad = SDIO_CS_PAD;
    SPI3_Dev.cs_pin = SDIO_CS_PNUM;
    spi_start(&SPI3_Dev, 0);
    mmc_start();
    GPIO_OFF(SDIO_PDOWN);
    vTaskDelay(pdMS_TO_TICKS(100));

    serial_puts(&Serial1, "Bootloader version: " VERSION "\r\n");
    rc = mmc_connect();
    if (rc == EERR_OK) {
        serial_puts(&Serial1, "SD connected\r\n");
    } else if (rc == EERR_TIMEOUT) {
        serial_puts(&Serial1, "Timed out waiting for SD\r\n");
        return;
    } else {
        serial_puts(&Serial1, "Failed to connect to SD\r\n");
        return;
    }

    serial_puts(&Serial1, "Mounting SD filesystem\r\n");
    if (f_mount(0, &MMC_FS) != FR_OK) {
        serial_puts(&Serial1, "ERROR: Unable to mount filesystem\r\n");
        return;
    }

    serial_puts(&Serial1, "Opening file " MMC_FIRMWARE_FILENAME "\r\n");
    if (f_open(&fp, MMC_FIRMWARE_FILENAME, FA_READ) != FR_OK) {
        serial_puts(&Serial1, "Error opening file, maybe it does not exist\r\n");
        return;
    }

    serial_puts(&Serial1, "Comparing file to current flash contents\r\n");
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
            serial_puts(&Serial1, "Error flashing firmware: ");
            serial_puts(&Serial1, errmsg);
            serial_puts(&Serial1, "\r\n");
            break;
        }
    }

    if (bootloader_status == BLS_DONE) {
        if (bootloader_was_changed()) {
            serial_puts(&Serial1, "New firmware successfully loaded\r\n");
        } else {
            serial_puts(&Serial1, "Firmware is up-to-date\r\n");
        }
    } else {
        serial_puts(&Serial1, "ERROR: Reset to try again or load last known good firmware\r\n");
        HALT();
    }
}


void
main_thread(void *pdata) {
    try_flash();
    if (user_vtor[1] == 0xFFFFFFFF) {
        serial_puts(&Serial1, "No application loaded, trying to load again in 10 seconds\r\n");
        vTaskDelay(pdMS_TO_TICKS(10000));
        NVIC_SystemReset();
    } else {
        serial_puts(&Serial1, "Booting application\r\n");
        vTaskDelay(pdMS_TO_TICKS(250));
        reset_and_jump();
    }
}


void
main(void) {
    if (jump_token == JUMP_TOKEN) {
        jump_token = 0;
        jump_application();
    }
    setup_hsi();
    serial_start(&Serial1, 115200);
    ASSERT(xTaskCreate(main_thread, "main", MAIN_STACK_SIZE, NULL,
                THREAD_PRIO_MAIN, &thread_main));
    vTaskStartScheduler();
}
