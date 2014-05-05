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
#define EEPROM_I2C			(&I2C1_Dev)

#define EEPROM_PAGE_SIZE	8
#define EEPROM_PAGES		16
#define EEPROM_SIZE			(EEPROM_PAGES * EEPROM_PAGE_SIZE)

#define CFG_VERSION			2

#define EERR_BLANK			-20
#define EERR_UPGRADE		-21

#define FLAG_PPSEN			(1 << 0)
#define FLAG_GPSEXT			(1 << 1)
#define FLAG_GPSOUT			(1 << 2)
#define FLAG_NTPKEY_MD5		(1 << 3)
#define FLAG_NTPKEY_SHA1	(1 << 4)
#define FLAG_HOLDOVER_TEST	(1 << 5)


#pragma pack(push, 1)

/* First 8 bytes cannot be modified at runtime */
typedef struct {
	uint16_t hwver;
	uint8_t serial[6];
} snumv2_t;
#define EEPROM_CFG_OFFSET sizeof(snumv2_t)

/* Remainder is user-modifiable configuration */
typedef struct {
	uint16_t version;
	uint32_t ip_addr;
	uint32_t ip_gateway;
	uint32_t ip_netmask;
	uint32_t gps_baud_rate;
	uint8_t admin_key[8];
	uint16_t gps_listen_port;
	uint32_t syslog_ip;
	uint32_t flags;
	uint32_t ip_manycast;
	uint8_t ntp_key[20];
	uint32_t holdover;
	uint8_t _reserved[54];
	uint16_t crc;
} cfgv2_t;
#define CFG_SIZE sizeof(cfgv2_t)

#pragma pack(pop)


extern snumv2_t snum;
extern cfgv2_t cfg;


int16_t eeprom_read(const uint8_t addr, uint8_t *buf, const uint8_t len);
int16_t eeprom_read_cfg(void);
int16_t eeprom_write_page(uint8_t addr, const uint8_t *buf);
int16_t eeprom_write_cfg(void);

#endif
