/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

#define FLASH_ERROR         40

#define BLS_WAITING         0
#define BLS_FLASHING        1
#define BLS_DONE            2
#define BLS_ERROR           3

extern uint8_t bootloader_status;

void bootloader_start(void);
const char *bootloader_feed(const uint8_t *buf, uint16_t size);
int bootloader_was_changed(void);

#endif
