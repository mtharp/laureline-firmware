/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "init.h"


void
SystemInit(void) {
	uint32_t n;
	/* Enable peripheral clocks */
	RCC->AHBENR |= 0
		| RCC_AHBENR_DMA1EN
		| RCC_AHBENR_DMA2EN
		| RCC_AHBENR_ETHMACEN
		| RCC_AHBENR_ETHMACTXEN
		| RCC_AHBENR_ETHMACRXEN
		;
	RCC->APB2ENR |= 0
		| RCC_APB2ENR_AFIOEN
		| RCC_APB2ENR_IOPAEN
		| RCC_APB2ENR_IOPBEN
		| RCC_APB2ENR_IOPCEN
		| RCC_APB2ENR_IOPDEN
		;

	/* Pin mapping */
	AFIO->MAPR |= 0
		| AFIO_MAPR_TIM3_REMAP_FULLREMAP
		| AFIO_MAPR_SWJ_CFG_JTAGDISABLE
		;

	/* Drive mux to select onboard TCXO */
#if CKSEL_PNUM < 8
	n = CKSEL_PAD->CRL;
	n &= ~(0xFFFF << (CKSEL_PNUM*4));
	n |= 0b0010 << (CKSEL_PNUM*4);
	CKSEL_PAD->CRL = n;
#else
	n = CKSEL_PAD->CRH;
	n &= ~(0xFFFF << ((CKSEL_PNUM-8)*4));
	n |= 0b0010 << ((CKSEL_PNUM-8)*4);
	CKSEL_PAD->CRH = n;
#endif
	CKSEL_PAD->BRR = CKSEL_PIN;

	/* Configure clocking */
	RCC->CR |= RCC_CR_HSEBYP | RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY)) {}
	RCC->CFGR = 0
		| RCC_CFGR_PLLSRC_PREDIV1 \
		| RCC_CFGR_HPRE_DIV1 \
		| RCC_CFGR_PPRE1_DIV2 \
		| RCC_CFGR_PPRE2_DIV1 \
		| RCC_CFGR_ADCPRE_DIV8;
	RCC->CFGR2 = RCC_CFGR2_PREDIV1SRC_HSE;
	FLASH->ACR = FLASH_ACR_LATENCY_2 | FLASH_ACR_PRFTBE;
}
