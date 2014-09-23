/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "stm32/flash.h"


static int
flash_unlock(void) {
    if (!(FLASH->CR & FLASH_CR_LOCK)) {
        /* Already unlocked */
        return 0;
    }

    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    if (!(FLASH->CR & FLASH_CR_LOCK)) {
        return 0;
    } else {
        return 1;
    }
}


static void
flash_lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}


static void
flash_wait_busy(void) {
    while (FLASH->SR & FLASH_SR_BSY) {}
}


int
flash_can_write(void *page) {
    if (page < (void*)_user_start
            || NEXT_PAGE(page) > (void*)_user_end) {
        return 0;
    } else {
        return 1;
    }
}


int
flash_page_erase(void *page) {
    ASSERT_ALIGNED(page);
    if (!flash_can_write(page)) {
        return FLASH_DENIED;
    }
    if (flash_unlock()) {
        return FLASH_FAULT;
    }
    flash_wait_busy();
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = (uint32_t)page;
    FLASH->CR |= FLASH_CR_STRT;
    flash_wait_busy();
    FLASH->CR &= ~FLASH_CR_PER;
    flash_lock();
    if (!flash_page_is_erased(page)) {
        return FLASH_FAULT;
    }
    return FLASH_OK;
}


int
flash_page_is_erased(void *page) {
    const uint32_t *ptr;
    const uint32_t *end = NEXT_PAGE(page);
    for (ptr = page; ptr < end; ptr++) {
        if (*ptr != 0xFFFFFFFF) {
            return 0;
        }
    }
    return 1;
}


int
flash_page_compare(void *page, const uint8_t *data) {
    const uint32_t *page_ptr, *data_ptr;
    const uint32_t *end = NEXT_PAGE(page);
    int ret = 0;
    page_ptr = (const uint32_t*)page;
    data_ptr = (const uint32_t*)data;
    for (; page_ptr < end; page_ptr++, data_ptr++) {
        if (*page_ptr == *data_ptr) {
            continue;
        }
        /* Not identical */
        if (ret == 0) {
            ret = 1;
        }
        if (*page_ptr != 0xFFFFFFFF) {
            /* Also not empty */
            ret = 2;
        }
    }
    return ret;
}


int
flash_page_write(void *page, const uint8_t *data) {
    volatile uint16_t *page_ptr = page;
    const uint16_t *page_end = NEXT_PAGE(page);
    const uint16_t *data_ptr = (const uint16_t *)data;
    ASSERT_ALIGNED(page);
    if (!flash_can_write(page)) {
        return FLASH_DENIED;
    }
    if (flash_unlock()) {
        return FLASH_FAULT;
    }
    flash_wait_busy();
    for (; page_ptr < page_end; page_ptr++, data_ptr++) {
        FLASH->CR |= FLASH_CR_PG;
        *page_ptr = *data_ptr;
        flash_wait_busy();
        FLASH->CR &= ~FLASH_CR_PG;
    }
    flash_lock();
    if (flash_page_compare(page, data) != 0) {
        return FLASH_FAULT;
    }
    return FLASH_OK;
}


int
flash_page_maybe_write(void *page, const uint8_t *data) {
    int status;
    status = flash_page_compare(page, data);
    if (status == 0) {
        /* Already identical */
        return FLASH_UNCHANGED;
    }
    if (status == 2) {
        /* Needs an erase */
        status = flash_page_erase(page);
        if (status) {
            return status;
        }
    }
    return flash_page_write(page, data);
}
