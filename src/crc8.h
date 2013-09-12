/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _CRC8_H
#define _CRC8_H

extern const uint8_t tbl_crc8[256];

#define CRC8(crc, byte) tbl_crc8[(crc) ^ (byte)]

#endif
