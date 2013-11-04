/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "ihex.h"

static uint32_t ihex_base;
static uint8_t ihex_buf[70];
static uint8_t ihex_cksum;
static uint16_t ihex_count;
static enum {
	START,
	COPY1,
	COPY2
} ihex_state;


#define REC_DATA			0
#define REC_EOF				1
#define REC_SEG_ADDR		2
#define REC_SEG_START		3
#define REC_LIN_ADDR		4
#define REC_LIN_START		5


void
ihex_init(void) {
	ihex_state = START;
	ihex_count = 0;
}


uint8_t
ihex_feed(const uint8_t *inbuf, uint16_t insize, ihex_cb callback) {
	uint8_t dat, rsize, rtype, rv;
	uint32_t addr;
	while (insize) {
		dat = *inbuf++;
		insize--;
		if (dat == '\r' || dat == '\n' || dat == '\0') {
			/* End of record */
			if (ihex_state == START) {
				/* \r\n or extra newline.
				 * netascii mode also likes to insert NULs.
				 */
				continue;
			}
			ihex_state = START;
			if (ihex_count < 5) {
				return IHEX_INVALID;
			}
			rsize = ihex_buf[0];
			addr = (ihex_buf[1] << 8) | ihex_buf[2];
			rtype = ihex_buf[3];
			if (ihex_count != 5 + rsize) {
				return IHEX_INVALID;
			}
			if (ihex_cksum != 0) {
				return IHEX_CHECKSUM;
			}

			switch (rtype) {
			case REC_DATA:
				addr += ihex_base;
				rv = callback(addr, &ihex_buf[4], rsize);
				if (rv != 0) {
					return rv;
				}
				break;
			case REC_EOF:
				return IHEX_EOF;
			case REC_SEG_ADDR:
				if (rsize != 2) {
					return IHEX_INVALID;
				}
				/* Shift left by 4 to get new base */
				ihex_base = (ihex_buf[4] << 12) | (ihex_buf[5] << 4);
				break;
			case REC_LIN_ADDR:
				if (rsize != 2) {
					return IHEX_INVALID;
				}
				/* Shift left by 16 to get new base */
				ihex_base = (ihex_buf[4] << 24) | (ihex_buf[5] << 16);
				break;
			case REC_SEG_START:
			case REC_LIN_START:
				/* Ignore the start address */
				break;
			default:
				return IHEX_UNSUPPORTED;
			}

		} else if (ihex_state == START) {
			/* Start code */
			if (dat != ':') {
				return IHEX_INVALID;
			}
			ihex_state = COPY1;
			ihex_cksum = 0;
			ihex_count = 0;
		} else {
			if (dat >= '0' && dat <= '9') {
				dat -= '0';
			} else if (dat >= 'A' && dat <= 'F') {
				dat -= 'A' - 10;
			} else if (dat >= 'a' && dat <= 'f') {
				dat -= 'a' - 10;
			} else {
				return IHEX_INVALID;
			}
			if (ihex_state == COPY1) {
				/* First, most-significant nybble */
				if (ihex_count >= sizeof(ihex_buf)) {
					return IHEX_UNSUPPORTED;
				}
				ihex_buf[ihex_count] = dat << 4;
				ihex_state = COPY2;
			} else {
				/* Second, least-significant nybble */
				ihex_buf[ihex_count] |= dat;
				ihex_cksum += ihex_buf[ihex_count];
				ihex_count++;
				ihex_state = COPY1;
			}
		}
	}
	return IHEX_CONTINUE;
}
