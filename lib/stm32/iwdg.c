/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "stm32/iwdg.h"


void
iwdg_start(uint8_t prescaler, uint16_t reload) {
	/* Stop IWDG when halted */
	DBGMCU->CR |= DBGMCU_CR_DBG_IWDG_STOP;

	IWDG->KR = IWDG_KEY_UNLOCK;
	IWDG->PR = prescaler;
	IWDG->RLR = reload;
	IWDG->KR = IWDG_KEY_START;
}


void
iwdg_clear(void) {
	IWDG->KR = IWDG_KEY_CLEAR;
}
