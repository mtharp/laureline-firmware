/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "bootloader.h"
#include "ihex.h"
#include "stm32/flash.h"


uint8_t bootloader_status;
static uint8_t dirty;
static void *current_page;
static uint8_t page_buffer[FLASH_PAGE_SIZE];


static uint8_t
bootloader_callback(uint32_t address, const uint8_t *data, uint16_t size) {
	void *addr_ptr = (void*)address;
	void *new_page;
	int index, chunk_size, rv;
	while (size > 0) {
		new_page = PAGE_OF(addr_ptr);
		if (new_page != current_page) {
			/* Write out contents for previous page */
			if (dirty) {
				rv = flash_page_maybe_write(current_page, page_buffer);
				if (rv != FLASH_OK) {
					break;
				}
			}
			/* Populate buffer with current contents of new page */
			memcpy(page_buffer, new_page, FLASH_PAGE_SIZE);
			dirty = 0;
			current_page = new_page;
		}
		index = (int)(addr_ptr - new_page);
		chunk_size = FLASH_PAGE_SIZE - index;
		if (chunk_size > size) {
			chunk_size = size;
		}
		memcpy(page_buffer + index, data, chunk_size);
		size -= chunk_size;
		data += chunk_size;
		dirty = 1;
	}
	return 0;
}


void
bootloader_start(void) {
	bootloader_status = BLS_FLASHING;
	current_page = 0;
	dirty = 0;
	ihex_init();
}


const char *
bootloader_feed(const uint8_t *buf, uint16_t size) {
	uint8_t rv;
	rv = ihex_feed(buf, size, bootloader_callback);
	if (rv == IHEX_EOF) {
		if (dirty) {
			rv = flash_page_maybe_write(current_page, page_buffer);
			if (rv == FLASH_OK) {
				bootloader_status = BLS_DONE;
				return NULL;
			}
		} else {
			bootloader_status = BLS_DONE;
			return NULL;
		}
	} else if (rv == IHEX_CONTINUE) {
		return NULL;
	}
	bootloader_status = BLS_ERROR;
	switch (rv) {
		case IHEX_UNSUPPORTED:
			return "unsupported ihex option";
		case IHEX_CHECKSUM:
			return "ihex checksum failure";
		case FLASH_FAULT:
			return "flash error";
		case FLASH_DENIED:
			return "application conflicts with bootloader";
		case IHEX_INVALID:
		default:
			return "malformed ihex";
	}
}
