/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "vectors.h"

/* These variables are placeholders which the linker will define. Only their
 * address is meaningful, their value is not. */

/* Bounds of data (preinitialized part of RAM) */
extern uint32_t _sdata;
extern uint32_t _edata;
/* Start of ROM where data is loaded from */
extern uint32_t _sidata;
/* Bounds of BSS (zeroed part of RAM) */
extern uint32_t _sbss;
extern uint32_t _ebss;

void SystemInit(void);
void main(void);


__attribute__((naked)) void
Reset_Handler(void) {
    asm volatile ("cpsid i");
    asm volatile ("msr MSP, %0" : : "r" (_isr_vector[0]));
    SCB->VTOR = (uint32_t)&_isr_vector;
    {
        uint32_t *ptr;
        for (ptr = &_sbss; ptr < &_ebss; ptr++) {
            *ptr = 0;
        }
    }
    {
        uint32_t *textptr, *dataptr;
        dataptr = &_sdata;
        textptr = &_sidata;
        while (dataptr < &_edata) {
            *dataptr++ = *textptr++;
        }
    }
    asm volatile ("cpsie i");

    SystemInit();
    main();

    while (1) {}
}
