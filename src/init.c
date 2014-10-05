/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#include "common.h"
#include "init.h"

uint32_t system_frequency;

#define DIV_MIN     1
#define DIV_MAX     16
#define MUL_MIN     4
#define MUL_MAX     10
/* 4-9 and 6.5 are the available multipliers; 10 represents 6.5 */

#define DIV1_MIN     3000000
#define DIV1_MAX    12000000
#define PLL_MIN     18000000
#define PLL_MAX     72000000


void
setup_clocks(uint32_t hse_freq) {
    uint32_t best_div, best_mul, best_freq;
    uint32_t reg;
    ASSERT(hse_freq > 1000000);
    ASSERT(hse_freq < 50000000);

    best_freq = best_div = best_mul = 0;
    for (int div = DIV_MIN; div <= DIV_MAX; div++) {
        /* Check PLL input clock */
        uint32_t div1 = hse_freq / div;
        if (div1 < DIV1_MIN || div1 > DIV1_MAX) {
            continue;
        }
        for (int mul = MUL_MIN; mul <= MUL_MAX; mul++) {
            /* Check PLL output clock */
            uint32_t freq;
            if (mul == 10) {
                freq = hse_freq * 13 / 2 / div; /* times 6.5 */
            } else {
                freq = hse_freq * mul / div;
            }
            if (freq < PLL_MIN || freq > PLL_MAX) {
                continue;
            }
            /* See if it beats the previous best */
            if (freq < best_freq) {
                continue;
            }
            best_freq = freq;
            best_div = div;
            best_mul = mul;
        }
    }
    if (best_freq == 0) {
        HALT();
    }

    /* Switch to HSI */
    if ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
        SET_BITS(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSI);
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {}
    }
    /* Disable PLL */
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) {}
    /* Configure routing */
    SET_BITS(RCC->CFGR, RCC_CFGR_PLLSRC, RCC_CFGR_PLLSRC_PREDIV1);
    SET_BITS(RCC->CFGR2, RCC_CFGR2_PREDIV1SRC, RCC_CFGR2_PREDIV1SRC_HSE);
    /* Configure multiplier (RCC_CFGR) */
    if (best_mul == 10) {
        reg = RCC_CFGR_PLLMULL6_5;
    } else {
        reg = (best_mul - 2) << 18;
    }
    SET_BITS(RCC->CFGR, RCC_CFGR_PLLMULL, reg);
    /* Configure divider (RCC_CFGR2) */
    SET_BITS(RCC->CFGR2, RCC_CFGR2_PREDIV1, best_div - 1);
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {}
    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    /* Switch to PLL */
    SET_BITS(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
    system_frequency = best_freq;
}


void setup_hsi(void) {
    /* Switch to HSI */
    if ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
        SET_BITS(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSI);
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {}
    }
#if 0
    /* Disable PLL */
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) {}
    /* Configure routing and PLL */
    SET_BITS(RCC->CFGR, RCC_CFGR_PLLSRC, RCC_CFGR_PLLSRC_HSI_Div2);
    SET_BITS(RCC->CFGR, RCC_CFGR_PLLMULL, (9UL - 2) << 18);
    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    /* Switch to PLL */
    SET_BITS(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    system_frequency = 36000000;
#else
    system_frequency = 8000000;
#endif
}


void
setup_gpio(GPIO_TypeDef *bank, int pin, int flags, int value) {
    uint32_t mask;
    volatile uint32_t *reg;
    if (value) {
        bank->BSRR = 1 << pin;
    } else {
        bank->BRR = 1 << pin;
    }
    if (pin < 8) {
        reg = &bank->CRL;
    } else {
        reg = &bank->CRH;
        pin -= 8;
    }
    mask = 0xF << (pin * 4);
    flags <<= (pin * 4);
    *reg = (*reg & ~mask) | flags;
}


void
setup_bank(GPIO_TypeDef *bank, const gpio_cfg_t *pins) {
    int i;
    uint32_t crl = 0, crh = 0, bsrr = 0;
    for (i = 0; i < 16; i++) {
        if (i < 8) {
            crl |= pins[i].flags << (4 * i);
        } else {
            crh |= pins[i].flags << (4 * (i - 8));
        }
        if (pins[i].value) {
            bsrr |= (1 << i);
        } else {
            bsrr |= (1 << (i + 16));
        }
    }
    bank->BSRR = bsrr;
    bank->CRL = crl;
    bank->CRH = crh;
}
