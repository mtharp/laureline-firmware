/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"


uint16_t status_flags;


void
set_status(uint16_t mask) {
	__disable_irq();
	status_flags |= mask;
	__enable_irq();
}


void
clear_status(uint16_t mask) {
	__disable_irq();
	status_flags &= ~mask;
	__enable_irq();
}
