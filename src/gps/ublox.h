/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _UBLOX_H
#define _UBLOX_H

#include "stm32/serial.h"

uint8_t ublox_feed(uint8_t data);
void ublox_configure(serial_t *ch);
void ublox_stop(serial_t *ch);

#endif
