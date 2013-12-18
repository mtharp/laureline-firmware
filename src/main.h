/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _MAIN_H
#define _MAIN_H

#include "stm32/serial.h"

extern serial_t *const gps_serial;

void log_startup(void);
void enter_standby(void);

#endif
