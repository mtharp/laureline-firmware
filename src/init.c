/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#include "common.h"
#include "init.h"


#ifdef BOOTLOADER
uint32_t system_frequency;
#else
double system_frequency;
#endif


void setup_clocks(double hse_freq) {
    int div, mul;
    int best_div, best_mul;
    uint32_t reg;
    double best_freq;
    double f;
    ASSERT(hse_freq > 1000000);
    ASSERT(hse_freq < 50000000);

    best_freq = best_div = best_mul = 0;
    for (div = 1; div <= 16; div++) {
        /* Check PLL input clock */
        f = hse_freq / div;
        if (f < 3e6 || f > 12e6) {
            continue;
        }
        for (mul = 4; mul <= 10; mul++) {
            /* Check PLL output clock */
            if (mul == 10) {
                f = hse_freq / div * 6.5;
            } else {
                f = hse_freq / div * mul;
            }
            if (f < 18e6 || f > 72e6) {
                continue;
            }
            /* See if it beats the previous best */
            if (f < best_freq) {
                continue;
            }
            best_freq = f;
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

    SysTick->LOAD = system_frequency / CFG_SYSTICK_FREQ - 1;
    SysTick->VAL = 0;
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
    SysTick->LOAD = 36000000 / CFG_SYSTICK_FREQ - 1;
#else
    system_frequency = 8000000;
    SysTick->LOAD = 8000000 / CFG_SYSTICK_FREQ - 1;
#endif
    SysTick->VAL = 0;
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
