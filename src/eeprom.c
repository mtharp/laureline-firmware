/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include <string.h>
#include "common.h"
#include "task.h"

#include "eeprom.h"
#include "lwip/inet_chksum.h"
#include "stm32/i2c.h"


snumv2_t snum;
cfgv2_t cfg;
static uint8_t * const cfg_bytes = (uint8_t * const)&cfg;


static int16_t
eeprom_do(const uint8_t *txbuf, size_t txbytes,
        uint8_t *rxbuf, size_t rxbytes) {
    int16_t status;
    uint8_t retries = 5;
    while (retries) {
        status = i2c_transact(EEPROM_I2C, (EEPROM_ADDR << 1),
                (uint8_t*)txbuf, txbytes);
        if (status == EERR_AGAIN) {
            /* Retry forever. */
            continue;
        } else if (status != EERR_OK) {
            retries--;
            continue;
        }
        if (rxbytes == 0) {
            break;
        }

        status = i2c_transact(EEPROM_I2C, (EEPROM_ADDR << 1) | 1,
                rxbuf, rxbytes);
        if (status != EERR_OK) {
            retries--;
            continue;
        } else {
            break;
        }
    }
    return status;
}


int16_t
eeprom_read(const uint8_t addr, uint8_t *buf, const uint8_t len) {
    int16_t status;
    i2c_start(EEPROM_I2C);
    status = eeprom_do(&addr, 1, buf, len);
    i2c_stop(EEPROM_I2C);
    return status;
}


int16_t
eeprom_write_page(uint8_t addr, const uint8_t *buf) {
    uint8_t i;
    uint8_t tmp[1 + EEPROM_PAGE_SIZE];
    TickType_t timeout;
    int16_t status;
    tmp[0] = addr;
    memcpy(&tmp[1], buf, EEPROM_PAGE_SIZE);
    i2c_start(EEPROM_I2C);
    for (i = 3; i; i--) {
        /* Write EEPROM */
        status = eeprom_do((uint8_t*)tmp, 1 + EEPROM_PAGE_SIZE, NULL, 0);
        if (status != EERR_OK) {
            goto cleanup;
        }
        /* Readback EEPROM and compare */
        timeout = xTaskGetTickCount() + pdMS_TO_TICKS(250);
        while (1) {
            if (xTaskGetTickCount() > timeout) {
                status = EERR_TIMEOUT;
                goto cleanup;
            }
            status = eeprom_do(&addr, 1, (uint8_t*)tmp, EEPROM_PAGE_SIZE);
            if (status == EERR_OK) {
                /* Done */
                break;
            } else if (status != EERR_NACK) {
                goto cleanup;
            }
            /* EEPROM is still writing */
        }
        status = 0;
        if (memcmp(tmp, buf, EEPROM_PAGE_SIZE) == 0) {
            /* Confirmed valid */
            break;
        }
        /* Read doesn't match write, try again */
    }
    /* 3 tries and still can't get it right */
    if (i == 0) {
        status = EERR_FAULT;
        goto cleanup;
    }
    status = EERR_OK;
cleanup:
    i2c_stop(EEPROM_I2C);
    return status;
}

int16_t
eeprom_read_cfg(void) {
    uint8_t i;
    int16_t status;
    i2c_start(EEPROM_I2C);
    for (i = 0; i < EEPROM_SIZE; i += EEPROM_PAGE_SIZE) {
        status = eeprom_do(&i, 1,
            i == 0 ? (void*)&snum : (void*)&cfg_bytes[i-EEPROM_CFG_OFFSET],
            EEPROM_PAGE_SIZE);
        if (status != EERR_OK) {
            i2c_stop(EEPROM_I2C);
            return status;
        }
    }
    i2c_stop(EEPROM_I2C);
    /* Validate checksum of user data */
    if (inet_chksum(&cfg, sizeof(cfg) - 2) != cfg.crc) {
        return EERR_CRCFAIL;
    }
    return EERR_OK;
}


int16_t
eeprom_write_cfg(void) {
    uint8_t addr;
    int16_t status;
    cfg.crc = inet_chksum(&cfg, sizeof(cfg) - 2);
    for (addr = EEPROM_CFG_OFFSET; addr < EEPROM_SIZE; addr += EEPROM_PAGE_SIZE) {
        status = eeprom_write_page(addr, &cfg_bytes[addr - EEPROM_CFG_OFFSET]);
        if (status != EERR_OK) {
            return status;
        }
    }
    return EERR_OK;
}
