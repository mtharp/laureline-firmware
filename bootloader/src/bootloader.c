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
static uint8_t dirty, changed;
static void *current_page;
static uint8_t page_buffer[FLASH_PAGE_SIZE];


static int
flush_page(void) {
    int rv;
    if (!dirty) {
        return 0;
    }
    rv = flash_page_maybe_write(current_page, page_buffer);
    if (rv == FLASH_OK) {
        changed = 1;
    } else if (rv != FLASH_UNCHANGED) {
        return rv;
    }
    dirty = 0;
    return 0;
}


static uint8_t
bootloader_callback(uint32_t address, const uint8_t *data, uint16_t size) {
    uint8_t *addr_ptr = (uint8_t*)address;
    void *new_page;
    int index, chunk_size, rv = 0;
    while (size > 0) {
        new_page = PAGE_OF(addr_ptr);
        if (new_page != current_page) {
            rv = flush_page();
            if (rv) {
                break;
            }
            /* Populate buffer with current contents of new page */
            memcpy(page_buffer, new_page, FLASH_PAGE_SIZE);
            dirty = 0;
            current_page = new_page;
        }
        index = (int)(addr_ptr - (uint8_t*)new_page);
        chunk_size = FLASH_PAGE_SIZE - index;
        if (chunk_size > size) {
            chunk_size = size;
        }
        memcpy(page_buffer + index, data, chunk_size);
        size -= chunk_size;
        data += chunk_size;
        addr_ptr += chunk_size;
        dirty = 1;
    }
    return rv;
}


void
bootloader_start(void) {
    bootloader_status = BLS_FLASHING;
    current_page = 0;
    dirty = changed = 0;
    ihex_init();
}


const char *
bootloader_feed(const uint8_t *buf, uint16_t size) {
    uint8_t rv;
    rv = ihex_feed(buf, size, bootloader_callback);
    if (rv == IHEX_EOF) {
        rv = flush_page();
        if (rv == 0) {
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


int
bootloader_was_changed(void) {
    return changed;
}
