/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _IWDG_H
#define _IWDG_H

void iwdg_start(uint8_t prescaler, uint16_t reload);

#define IWDG_KEY_UNLOCK     0x5555
#define IWDG_KEY_CLEAR      0xAAAA
#define IWDG_KEY_START      0xCCCC


static inline void
iwdg_clear(void) {
    IWDG->KR = IWDG_KEY_CLEAR;
}

#endif
