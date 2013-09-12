/*
 * Copyright Michael Tharp <gxti@partiallystapled.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _EEPROM_H
#define _EEPROM_H

#define EEPROM_ADDR			0b1010000
#define EEPROM_I2C			I2C1

#define EEPROM_CFG_PAGES	16
#define EEPROM_CFG_SIZE		(EEPROM_CFG_PAGES*8)

#define CFG_VERSION			1

#define EERR_OK				0
#define EERR_TIMEOUT		-1
#define EERR_NACK			-2
#define EERR_CRCFAIL		-3
#define EERR_FAULT			-4
#define EERR_BLANK			-5
#define EERR_UPGRADE		-6
#define EERR_ARLO			-20


#pragma pack(push, 0)

typedef struct {
	/* First chunk */
	uint32_t ip_addr;
	uint32_t ip_gateway;
	uint32_t ip_netmask;
	uint16_t version;
	uint8_t _reserved;
	uint8_t crc1;
	/* Second chunk */
	uint32_t gps_baud_rate;
	uint8_t _reserved2[107];
	uint8_t crc;
} cfgv1_t;

#pragma pack(pop)


extern cfgv1_t cfg;
extern uint8_t * const cfg_bytes;


int16_t eeprom_read(const uint8_t addr, uint8_t *buf, const uint8_t len);
int16_t eeprom_read_cfg(void);
int16_t eeprom_write_cfg(void);
int16_t eeprom_erase(void);

#endif
