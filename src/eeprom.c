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

OS_EventID i2c_sem = E_CREATE_FAIL;
static uint8_t i2c_addr_dir;
static uint8_t *i2c_buf;
static uint8_t i2c_count;
static uint8_t i2c_index;
static uint8_t i2c_error;

static void i2c_configure(I2C_TypeDef *d);

#define FENCE() asm volatile("dmb" ::: "memory")


static void
handle_i2c_event(I2C_TypeDef *d) {
	uint16_t timeout;
	uint16_t sr1 = d->SR1;
	if (sr1 & I2C_SR1_SB) {
		/* Start bit is sent, now send address */
		d->CR1 |= I2C_CR1_ACK;
		i2c_index = 0;
		if ((i2c_addr_dir & 1) && i2c_count == 2) {
			/* Give advance notice of NACK after a 2-byte read */
			d->CR1 |= I2C_CR1_POS;
		}
		d->DR = i2c_addr_dir;

	} else if (sr1 & I2C_SR1_ADDR) {
		/* Address is sent, now send data */
		FENCE();
		if ((i2c_addr_dir & 1) && i2c_count == 1) {
			/* Receiving 1 byte */
			d->CR1 &= ~I2C_CR1_ACK;
			FENCE();
			(void)d->SR2; /* clear ADDR */
			d->CR1 |= I2C_CR1_STOP;
			d->CR2 |= I2C_CR2_ITBUFEN;
		} else {
			(void)d->SR2; /* clear ADDR */
			FENCE();
			if ((i2c_addr_dir & 1) && i2c_count == 2) {
				/* Receiving 2 bytes */
				d->CR1 &= ~I2C_CR1_ACK;
				d->CR2 &= ~I2C_CR2_ITBUFEN;
			} else if ((i2c_addr_dir & 1) && i2c_count == 3) {
				/* Receiving 3 bytes */
				d->CR2 &= ~I2C_CR2_ITBUFEN;
			} else {
				/* Receiving 4+ bytes, or transmitting */
				d->CR2 |= I2C_CR2_ITBUFEN;
			}
		}

	} else if (sr1 & I2C_SR1_BTF) {
		/* Byte transfer finished */
		if (i2c_addr_dir & 1) {
			if (i2c_count > 2) {
				/* Normal receive */
				d->CR1 &= ~I2C_CR1_ACK;
				i2c_buf[i2c_index++] = d->DR;
				d->CR1 |= I2C_CR1_STOP;
				i2c_buf[i2c_index++] = d->DR;
				d->CR2 |= I2C_CR2_ITBUFEN;
			} else {
				/* Short receive */
				d->CR1 |= I2C_CR1_STOP;
				i2c_buf[i2c_index++] = d->DR;
				i2c_buf[i2c_index++] = d->DR;
				i2c_index++; /* done */
			}
		} else {
			/* Transmit complete */
			d->CR1 |= I2C_CR1_STOP;
			i2c_index++; /* done */
		}

	} else if (sr1 & I2C_SR1_RXNE) {
		i2c_buf[i2c_index++] = d->DR;
		if (i2c_index + 3 == i2c_count) {
			/* Disable TXE to allow the buffer to flush */
			d->CR2 &= ~I2C_CR2_ITBUFEN;
		} else if (i2c_index == i2c_count) {
			i2c_index++; /* done */
		}

	} else if (sr1 & I2C_SR1_TXE) {
		/* Byte transmitted */
		d->DR = i2c_buf[i2c_index++];
		if (i2c_index == i2c_count) {
			/* Disable TXE to allow the buffer to flush */
			d->CR2 &= ~I2C_CR2_ITBUFEN;
		}
	}

	if (i2c_index == i2c_count + 1) {
		/* Done, disable interrupts until next transaction */
		d->CR1 &= ~I2C_CR1_POS;
		d->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | I2C_CR2_ITBUFEN);
		timeout = 15000;
		while (d->CR1 & (I2C_CR1_START | I2C_CR1_STOP)) {
			if (--timeout == 0) {
				i2c_error = EERR_TIMEOUT;
			}
		}
		/* Wake up userspace */
		isr_PostSem(i2c_sem);
	}
}


static void
handle_i2c_error(I2C_TypeDef *d) {
	uint16_t sr1 = d->SR1;
	(void)d->SR2;
	if (sr1 & I2C_SR1_OVR) {
		i2c_error = EERR_FAULT;
	} else if (sr1 & I2C_SR1_ARLO) {
		i2c_error = EERR_ARLO;
		d->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | I2C_CR2_ITBUFEN);
	} else if (sr1 & (I2C_SR1_AF | I2C_SR1_BERR)) {
		if (sr1 & I2C_SR1_AF) {
			i2c_error = EERR_NACK;
		} else {
			i2c_error = EERR_FAULT;
		}
		d->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | I2C_CR2_ITBUFEN);
		if (d->CR1 & I2C_CR1_START) {
			/* START+STOP hangs the peripheral, so reset afterwards */
			while (d->CR1 & I2C_CR1_START) {}
			d->CR1 |= I2C_CR1_STOP;
			while (d->CR1 & I2C_CR1_STOP) {}
			i2c_configure(d);
		} else {
			d->CR1 |= I2C_CR1_STOP;
		}
	} else {
		/* No error. Why are we here? */
		return;
	}
	/* Clear errors and wake up userspace */
	isr_PostSem(i2c_sem);
	d->SR1 &= ~0x0F00;
}


void
I2C1_EV_IRQHandler(void) {
	CoEnterISR();
	handle_i2c_event(I2C1);
	CoExitISR();
}


void
I2C1_ER_IRQHandler(void) {
	CoEnterISR();
	handle_i2c_error(I2C1);
	CoExitISR();
}

static void
i2c_start(I2C_TypeDef *d) {
	if (i2c_mutex == E_CREATE_FAIL) {
		i2c_mutex = CoCreateMutex();
		if (i2c_mutex == E_CREATE_FAIL) { HALT(); }
	}
	if (i2c_sem == E_CREATE_FAIL) {
		i2c_sem = CoCreateSem(0, 1, EVENT_SORT_TYPE_FIFO);
		if (i2c_sem == E_CREATE_FAIL) { HALT(); }
	}
	CoEnterMutexSection(i2c_mutex);
	i2c_configure(d);
}


static void
i2c_configure(I2C_TypeDef *d) {
	if (d == I2C1) {
		RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
		RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
		RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;
		NVIC_EnableIRQ(I2C1_EV_IRQn);
		NVIC_EnableIRQ(I2C1_ER_IRQn);
	} else if (d == I2C2) {
		RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
		RCC->APB1RSTR |= RCC_APB1RSTR_I2C2RST;
		RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C2RST;
		NVIC_EnableIRQ(I2C2_EV_IRQn);
		NVIC_EnableIRQ(I2C2_ER_IRQn);
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
i2c_transact(I2C_TypeDef *d, uint8_t addr_dir,
		uint8_t *buf, size_t count) {
	i2c_addr_dir = addr_dir;
	i2c_buf = buf;
	i2c_count = count;
	i2c_error = 0;
	/* Begin the transaction */
	d->CR1 |= I2C_CR1_START;
	d->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;
	/* Wait for magic to happen */
	CoPendSem(i2c_sem, 0);
	/* Check if it worked */
	return i2c_error;
}


static int16_t
eeprom_do(const uint8_t *txbuf, size_t txbytes,
		uint8_t *rxbuf, size_t rxbytes) {
	int16_t status;
	uint8_t retries = 5;
	while (retries) {
		status = i2c_transact(EEPROM_I2C, (EEPROM_ADDR << 1),
				(uint8_t*)txbuf, txbytes);
		if (status == EERR_ARLO) {
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
