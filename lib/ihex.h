/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _IHEX_H
#define _IHEX_H

#include <stdint.h>

#define IHEX_CONTINUE		0
#define IHEX_EOF			30
#define IHEX_INVALID		31
#define IHEX_UNSUPPORTED	32
#define IHEX_CHECKSUM		33

typedef uint8_t(*ihex_cb)(uint32_t address, const uint8_t *data, uint16_t length);

void ihex_init(void);
uint8_t ihex_feed(const uint8_t *inbuf, uint16_t insize, ihex_cb callback);

#endif
