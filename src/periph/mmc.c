/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "crc7.h"
#include "init.h"
#include "periph/mmc.h"
#include "periph/spi.h"

#define MMCSPI (&SPI3_Dev)

static mmc_state_t mmc_state;
static uint8_t mmc_block_mode;


static void
mmc_ll_wait_idle(void) {
	int i;
	uint8_t result;
	U64 deadline;
	for (i = 0; i < 16; i++) {
		spi_exchange(MMCSPI, NULL, &result, 1);
		if (result == 0xFF) {
			return;
		}
	}
	deadline = CoGetOSTime() + MMC_IDLE_DEADLINE;
	while (1) {
		spi_exchange(MMCSPI, NULL, &result, 1);
		if (result == 0xFF) {
			return;
		}
		if (CoGetOSTime() > deadline) {
			return;
		}
		CoTickDelay(MS2ST(10));
	}
}


static void
mmc_ll_send_header(uint8_t cmd, uint32_t arg) {
	uint8_t buf[6];
	crc7_t crc;
	buf[0] = 0x40 | cmd;
	buf[1] = arg >> 24;
	buf[2] = arg >> 16;
	buf[3] = arg >>  8;
	buf[4] = arg;
	crc = crc7_init();
	crc = crc7_update(crc, buf, 5);
	buf[5] = (crc7_finalize(crc) << 1) | 0x01;
	spi_exchange(MMCSPI, buf, NULL, 6);
}


static uint8_t
mmc_ll_receive_r1(void) {
	int i;
	uint8_t r;
	for (i = 0; i < 9; i++) {
		spi_exchange(MMCSPI, NULL, &r, 1);
		if (r != 0xFF) {
			return r;
		}
	}
	return 0xFF;
}


static uint8_t
mmc_cmd_r1(uint8_t cmd, uint32_t arg) {
	uint8_t ret;
	spi_select(MMCSPI);
	if (cmd != MMC_CMDGOIDLE) {
		mmc_ll_wait_idle();
	}
	mmc_ll_send_header(cmd, arg);
	ret = mmc_ll_receive_r1();
	spi_deselect(MMCSPI);
	return ret;
}


static uint8_t
mmc_cmd_r3(uint8_t cmd, uint32_t arg, uint8_t *buf) {
	uint8_t ret;
	spi_select(MMCSPI);
	mmc_ll_wait_idle();
	mmc_ll_send_header(cmd, arg);
	ret = mmc_ll_receive_r1();
	spi_exchange(MMCSPI, NULL, buf, 4);
	spi_deselect(MMCSPI);
	return ret;
}


void
mmc_start(void) {
	mmc_state = MMC_UNLOADED;
	mmc_block_mode = 0;
}


int16_t
mmc_connect(void) {
	U64 deadline;
	uint8_t n, buf[4];

	/* Run SPI in slow mode and clear the bus by clocking a few bytes */
	spi_deselect(MMCSPI);
	MMCSPI->spi->CR1 &= ~SPI_CR1_SPE;
	MMCSPI->spi->CR1 |= SPI_CR1_SPE | SPI_CR1_BR_2 | SPI_CR1_BR_1;
	spi_exchange(MMCSPI, NULL, NULL, 16);

	/* Select SPI mode */
	deadline = CoGetOSTime() + MMC_RESET_DEADLINE;
	while (1) {
		if (mmc_cmd_r1(MMC_CMDGOIDLE, 0) == 0x01) {
			break;
		}
		if (CoGetOSTime() >= deadline) {
			mmc_state = MMC_UNLOADED;
			return EERR_TIMEOUT;
		}
		CoTickDelay(MS2ST(10));
	}

	/* Detect card type */
	if (mmc_cmd_r3(MMC_CMDINTERFACE_CONDITION, 0x01AA, buf) != 0x05) {
		/* Switch to v2 */
		deadline = CoGetOSTime() + MMC_INIT_DEADLINE;
		while (1) {
			if ((mmc_cmd_r1(MMC_CMDAPP, 0) == 0x01)
					&& (mmc_cmd_r3(MMC_ACMDOPCONDITION, 0x400001aa, buf) == 0x00)) {
				break;
			}
			if (CoGetOSTime() >= deadline) {
				mmc_state = MMC_UNLOADED;
				return EERR_TIMEOUT;
			}
			CoTickDelay(MS2ST(10));
		}
		/* Check for block mode */
		mmc_cmd_r3(MMC_CMDREADOCR, 0, buf);
		if (buf[0] & 0x40) {
			mmc_block_mode = 1;
		}
	} else {
		/* MMC or SDv1 */
		deadline = CoGetOSTime() + MMC_INIT_DEADLINE;
		while (1) {
			n = mmc_cmd_r1(MMC_CMDINIT, 0);
			if (n == 0x00) {
				break;
			} else if (n != 0x01) {
				mmc_state = MMC_UNLOADED;
				return EERR_FAULT;
			}
			if (CoGetOSTime() >= deadline) {
				mmc_state = MMC_UNLOADED;
				return EERR_TIMEOUT;
			}
			CoTickDelay(MS2ST(10));
		}
	}

	/* Full speed */
	MMCSPI->spi->CR1 &= ~(SPI_CR1_SPE | SPI_CR1_BR_2 | SPI_CR1_BR_1);
	MMCSPI->spi->CR1 |= SPI_CR1_SPE;

	/* Check block size */
	if (mmc_cmd_r1(MMC_CMDSETBLOCKLEN, 512) != 0) {
		mmc_state = MMC_UNLOADED;
		return EERR_FAULT;
	}

	mmc_state = MMC_READY;
	return EERR_OK;
}


int16_t
mmc_disconnect(void) {
	if (mmc_state == MMC_UNLOADED) {
		return EERR_OK;
	} else if (mmc_state != MMC_READY) {
		return EERR_INVALID;
	}
	mmc_sync();
	mmc_state = MMC_UNLOADED;
	/* Clear the bus */
	spi_exchange(MMCSPI, NULL, NULL, 16);
	return EERR_OK;
}


void
mmc_sync(void) {
	spi_select(MMCSPI);
	mmc_ll_wait_idle();
	spi_deselect(MMCSPI);
}


int16_t
mmc_start_read(uint32_t lba) {
	if (mmc_state != MMC_READY) {
		return EERR_INVALID;
	}
	mmc_state = MMC_READING;

	spi_select(MMCSPI);
	mmc_ll_wait_idle();
	if (mmc_block_mode != 0) {
		mmc_ll_send_header(MMC_CMDREADMULTIPLE, lba);
	} else {
		mmc_ll_send_header(MMC_CMDREADMULTIPLE, lba * 512);
	}
	uint8_t rc = mmc_ll_receive_r1();
	if (rc != 0x00) {
		spi_deselect(MMCSPI);
		mmc_state = MMC_READY;
		return EERR_FAULT;
	}
	return EERR_OK;
}


int16_t
mmc_read_sector(uint8_t *out) {
	uint8_t r;
	uint16_t crc;
	U64 deadline;
	if (mmc_state != MMC_READING) {
		return EERR_INVALID;
	}
	deadline = CoGetOSTime() + MMC_DATA_DEADLINE;
	while (1) {
		spi_exchange(MMCSPI, NULL, &r, 1);
		if (r == 0xFE) {
			spi_exchange(MMCSPI, NULL, out, 512);
			spi_exchange(MMCSPI, NULL, NULL, 2);
			/* TODO: check CRC */
			return EERR_OK;
		}
		if (CoGetOSTime() >= deadline) {
			break;
		}
	}

	spi_deselect(MMCSPI);
	if (mmc_state == MMC_READING) {
		mmc_state = MMC_READY;
	}
	return EERR_TIMEOUT;
}


int16_t
mmc_stop_read(void) {
	static const uint8_t stop_cmd[] = {0x40 | MMC_CMDSTOP, 0, 0, 0, 0, 1, 0xFF};
	if (mmc_state != MMC_READING) {
		return EERR_INVALID;
	}
	spi_exchange(MMCSPI, stop_cmd, NULL, sizeof(stop_cmd));
	mmc_ll_receive_r1();
	spi_deselect(MMCSPI);
	mmc_state = MMC_READY;
	return EERR_OK;
}
