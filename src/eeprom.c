/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include <string.h>

#include "common.h"
#include "eeprom.h"
#include "crc8.h"
#include "stm32/i2c.h"


cfgv1_t cfg;
uint8_t * const cfg_bytes = (uint8_t * const)&cfg;


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
eeprom_read_cfg(void) {
	uint8_t i, crc;
	int16_t status;
	i2c_start(EEPROM_I2C);
	for (i = 0; i < EEPROM_CFG_SIZE; i += 8) {
		status = eeprom_do(&i, 1, &cfg_bytes[i], 8);
		if (status != EERR_OK) {
			i2c_stop(EEPROM_I2C);
			return status;
		}
	}
	i2c_stop(EEPROM_I2C);
	crc = 0xFF;
	status = 1;
	/* First 16 bytes have their own CRC since that's what the bootloader was
	 * originally written to.
	 */
	for (i = 0; i < 16; i++) {
		crc = CRC8(crc, cfg_bytes[i]);
		if (cfg_bytes[i] != 0xFF) {
			/* First segment is blank */
			status = 0;
		}
	}
	if (crc != 0) {
		if (status == 0) {
			/* Fail now if CRC doesn't match and first segment is not blank */
			return EERR_CRCFAIL;
		}
		/* First segment is blank */
		status = 1;
	} else {
		/* First segment matched but second segment might be blank when
		 * upgrading.
		 */
		status = 2;
	}
	/* Remaining bytes are one big chunk */
	crc = 0xFF;
	for (i = 16; i < EEPROM_CFG_SIZE; i++) {
		crc = CRC8(crc, cfg_bytes[i]);
		if (cfg_bytes[i] != 0xFF) {
			status = 0;
		}
	}
	if (crc != 0) {
		if (status == 2) {
			/* First segment OK, second segment blank */
			return EERR_UPGRADE;
		} else if (status == 1) {
			/* Both segments blank */
			return EERR_BLANK;
		} else {
			/* Either segment CRC failed and not blank */
			return EERR_CRCFAIL;
		}
	}
	return EERR_OK;
}


int16_t
eeprom_write_cfg(void) {
	uint8_t i, j, addr, crc;
	uint8_t tmp[9];
	uint64_t timeout;
	int16_t status;
	crc = 0xFF;
	for (i = 0; i < 15; i++) {
		crc = CRC8(crc, cfg_bytes[i]);
	}
	cfg_bytes[i] = crc;
	crc = 0xFF;
	for (i = 16; i < EEPROM_CFG_SIZE - 1; i++) {
		crc = CRC8(crc, cfg_bytes[i]);
	}
	cfg_bytes[i] = crc;

	i2c_start(EEPROM_I2C);
	for (addr = 0; addr < EEPROM_CFG_SIZE; addr += 8) {
		for (j = 3; j; j--) {
			tmp[0] = addr;
			for (i = 0; i < 8; i++) {
				tmp[1+i] = cfg_bytes[addr+i];
			}
			/* Write EEPROM */
			status = eeprom_do((uint8_t*)tmp, 9, NULL, 0);
			if (status != EERR_OK) {
				goto cleanup;
			}
			/* Readback EEPROM and compare */
			timeout = CoGetOSTime() + MS2ST(250);
			while (1) {
				if (CoGetOSTime() > timeout) {
					status = EERR_TIMEOUT;
					goto cleanup;
				}
				status = eeprom_do(&addr, 1, (uint8_t*)tmp, 8);
				if (status == EERR_OK) {
					/* Done */
					break;
				} else if (status != EERR_NACK) {
					goto cleanup;
				}
				/* EEPROM is still writing */
			}
			status = 0;
			for (i = 0; i < 8; i++) {
				if (tmp[i] != cfg_bytes[addr+i]) {
					status = 1;
					break;
				}
			}
			if (status == 0) {
				/* Everything checks out */
				break;
			}
			/* Read doesn't match write, try again */
		}
		/* 3 tries and still can't get it right */
		if (j == 0) {
			status = EERR_FAULT;
			goto cleanup;
		}
	}
	status = EERR_OK;
cleanup:
	i2c_stop(EEPROM_I2C);
	return status;
}


int16_t
eeprom_erase(void) {
	int16_t status;
	uint8_t addr, i;
	uint8_t tmp[8];
	uint8_t out[9];
	for (i = 1; i < 9; i++) {
		out[i] = 0xFF;
	}
	i2c_start(EEPROM_I2C);
	for (addr = 0x00; addr < 0x80; addr += 8) {
		/* Check if page is already blank */
		status = eeprom_do(&addr, 1, (uint8_t*)tmp, 8);
		if (status != EERR_OK) {
			goto cleanup;
		}
		for (i = 0; i < 8; i++) {
			if (tmp[i] != 0xFF) {
				status = 1;
				break;
			}
		}
		if (status == EERR_OK) {
			continue;
		}
		/* Erase page (write all-ones) */
		out[0] = addr;
		status = eeprom_do((uint8_t*)out, 9, NULL, 0);
		if (status != EERR_OK) {
			continue;
		}
		/* Wait for ACK */
		while (1) {
			status = eeprom_do(&addr, 1, (uint8_t*)tmp, 8);
			if (status == EERR_OK) {
				break;
			} else if (status != EERR_NACK) {
				goto cleanup;
			}
			/* Still writing */
		}
	}
	status = EERR_OK;
cleanup:
	i2c_stop(EEPROM_I2C);
	return status;
}
