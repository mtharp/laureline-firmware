/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#ifndef _FLASH_H
#define _FLASH_H

#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE 2048
#endif

#define FLASH_OK		0
#define FLASH_UNCHANGED	1
#define FLASH_DENIED	2
#define FLASH_FAULT		3

#define PAGE_OF(addr) ((void*)( ((uint32_t)(addr) / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE ))
#define NEXT_PAGE(addr) ((void*)( ((uint8_t*)(addr)) + FLASH_PAGE_SIZE ))
#define ASSERT_ALIGNED(addr) ASSERT(0 == ((uint32_t)(addr) % FLASH_PAGE_SIZE) )

extern uint32_t _user_start[];
extern uint32_t _user_end[];

int flash_page_is_erased(void *addr);
int flash_page_compare(void *page, const uint8_t *data);
int flash_page_erase(void *addr);
int flash_page_maybe_write(void *page, const uint8_t *data);
int flash_page_write(void *page, const uint8_t *data);

#endif
