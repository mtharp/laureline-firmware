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
#include "init.h"


cfgv1_t cfg;
OS_MutexID i2c_mutex = E_CREATE_FAIL;
uint8_t * const cfg_bytes = (uint8_t * const)&cfg;


static void
i2c_start(I2C_TypeDef *d) {
	if (i2c_mutex == E_CREATE_FAIL) {
		i2c_mutex = CoCreateMutex();
		if (i2c_mutex == E_CREATE_FAIL) { HALT(); }
	}
	CoEnterMutexSection(i2c_mutex);

	if (d == I2C1) {
		RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
		RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
		RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;
	} else if (d == I2C2) {
		RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
		RCC->APB1RSTR |= RCC_APB1RSTR_I2C2RST;
		RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C2RST;
	} else { HALT(); }
	d->CR1 = I2C_CR1_SWRST;
	d->CR1 = 0;
	/* FIXME: assuming APB1 = sysclk / 2 */
	d->CR2 = (uint16_t)(system_frequency/2 / 1e6);
	d->CCR = (uint16_t)((system_frequency/2) / 2 / 10e3);
	d->TRISE = (uint16_t)(1e-6 / (system_frequency/2) + 1);
	d->CR1 = I2C_CR1_PE;
}


static void
i2c_stop(I2C_TypeDef *d) {
	d->CR1 = 0;
	CoLeaveMutexSection(i2c_mutex);
}


static int16_t
i2c_transact(I2C_TypeDef *d, uint8_t addr, const uint8_t *txbuf, size_t
		txbytes, uint8_t *rxbuf, size_t rxbytes) {
	uint16_t sr1;
	while (d->SR2 & I2C_SR2_BUSY) {}
	d->CR1 |= I2C_CR1_START;
	while (!(d->SR1 & I2C_SR1_SB)) {}
	d->DR = addr << 1;
	/* reading SR1 then writing DR clears SB */
	while (1) {
		sr1 = d->SR1;
		(void)d->SR2;
		if (sr1 & I2C_SR1_AF) {
			d->CR1 |= I2C_CR1_STOP;
			return EERR_NACK;
		} else if (!(sr1 & I2C_SR1_ADDR)) {
			break;
		}
	}
	while (txbytes) {
		while (1) {
			/* reading SR1 then SR2 clears ADDR */
			sr1 = d->SR1;
			(void)d->SR2;
			if (sr1 & I2C_SR1_ARLO) {
				return EERR_ARLO;
			} else if (sr1 & I2C_SR1_AF) {
				d->CR1 |= I2C_CR1_STOP;
				return EERR_NACK;
			} else if (sr1 & I2C_SR1_TXE) {
				break;
			}
		}
		d->DR = *txbuf++;
		txbytes--;
	}
	while (1) {
		sr1 = d->SR1;
		(void)d->SR2;
		if (sr1 & I2C_SR1_BTF) {
			break;
		} else if (sr1 & I2C_SR1_AF) {
			d->CR1 |= I2C_CR1_STOP;
			return EERR_NACK;
		}
	}
	if (rxbytes) {
		d->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
		while (!(d->SR1 & I2C_SR1_SB)) {}
		d->DR = (addr << 1) | 0x01;
		while (1) {
			sr1 = d->SR1;
			(void)d->SR2;
			if (sr1 & I2C_SR1_AF) {
				d->CR1 |= I2C_CR1_STOP;
				return EERR_NACK;
			} else if (!(sr1 & I2C_SR1_ADDR)) {
				break;
			}
		}
		while (rxbytes--) {
			while (1) {
				sr1 = d->SR1;
				(void)d->SR2;
				if (sr1 & I2C_SR1_RXNE) {
					break;
				}
			}
			if (rxbytes == 0) {
				d->CR1 &= ~I2C_CR1_ACK;
			}
			*rxbuf++ = d->DR;
		}
	}
	d->CR1 |= I2C_CR1_STOP;
	return EERR_OK;
}


static int16_t
eeprom_do(const uint8_t *txbuf, size_t txbytes,
		uint8_t *rxbuf, size_t rxbytes) {
	int16_t status;
	uint8_t retries = 5;
	while (retries) {
		status = i2c_transact(EEPROM_I2C, EEPROM_ADDR,
				txbuf, txbytes, rxbuf, rxbytes);
		if (status == EERR_OK) {
			return EERR_OK;
		} else if (status == EERR_ARLO) {
			/* Retry forever. */
			continue;
		}
		retries--;
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
				/*if (CoGetOSTime() > timeout) {
					status = EERR_TIMEOUT;
					goto cleanup;
				}*/
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
