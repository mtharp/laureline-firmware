/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _EEPROM_H
#define _EEPROM_H

#define EEPROM_ADDR			0b1010000
#define EEPROM_I2C			I2C1

#define EEPROM_CFG_PAGES	16
#define EEPROM_CFG_SIZE		(EEPROM_CFG_PAGES*8)

#define CFG_VERSION			1

#define EERR_NACK			-20
#define EERR_CRCFAIL		-21
#define EERR_BLANK			-22
#define EERR_UPGRADE		-23
#define EERR_ARLO			-24


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
