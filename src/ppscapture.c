/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "eeprom.h"
#include "ppscapture.h"


/* High-order part of the monotonic timer */
static volatile uint64_t mono_epoch;
/* Last monotonic capture, or 0 if there are no pending captures */
static uint64_t mono_capture;
/* Next sleep, or 0 if none */
static uint64_t sleep_epoch;
static OS_FlagID sleep_flag;

/* Reduce this to 1500 to make it easier to hit edge cases to test the input
 * capture code */
#define MONO_PERIOD 65536


void
ppscapture_start(void) {
	ASSERT((sleep_flag = CoCreateFlag(1, 0)) != E_CREATE_FAIL);
	mono_epoch = MONO_PERIOD;
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->CR1 = 0;
	TIM3->PSC = 0;
	TIM3->ARR = MONO_PERIOD - 1;
	/* input capture 1 or 3 receives the pulse-per-second */
	TIM3->CCMR1 = TIM_CCMR1_CC1S_0;
	TIM3->CCMR2 = TIM_CCMR2_CC3S_0;
	TIM3->CCER = (cfg.flags & FLAG_GPSEXT) ? TIM_CCER_CC3E : TIM_CCER_CC1E;
	/* interrupt on update and input capture */
	TIM3->DIER = TIM_DIER_UIE | TIM_DIER_CC1IE | TIM_DIER_CC3IE;
	TIM3->SR = 0;

	NVIC_SetPriority(TIM3_IRQn, IRQ_PRIO_PPSCAPTURE);
	NVIC_EnableIRQ(TIM3_IRQn);
	TIM3->CR1 |= TIM_CR1_CEN;
}


void
TIM3_IRQHandler(void) {
	uint16_t sr, ccr;
	CoEnterISR();
	sr = TIM3->SR;
	TIM3->SR = ~sr;

	if (sr & TIM_SR_UIF) {
		mono_epoch += MONO_PERIOD;
		if (sleep_epoch != 0 && mono_epoch >= sleep_epoch) {
			sleep_epoch = 0;
			CoSetFlag(sleep_flag);
		}
	}

	if (sr & TIM_SR_CC1IF) {
		ccr = TIM3->CCR1;
	} else if (sr & TIM_SR_CC3IF) {
		ccr = TIM3->CCR3;
	} else {
		CoExitISR();
		return;
	}

	mono_capture = mono_epoch + ccr;
	if ((sr & TIM_SR_UIF) && ccr > (MONO_PERIOD / 2)) {
		/* This capture is from the previous period, but the update has already
		 * been processed above so undo its effect.
		 */
		mono_capture -= MONO_PERIOD;
	}
	CoExitISR();
}


uint64_t
monotonic_now(void) {
	/* Get value of monotonic clock */
	uint64_t ret;
	uint16_t tmr1, tmr2;
	DISABLE_IRQ();
	while (1) {
		tmr1 = TIM3->CNT;
		ret = mono_epoch;
		tmr2 = TIM3->CNT;
		if (tmr2 > tmr1) {
			break;
		}
		/* Timer rolled over while we were sampling. Process the update event
		 * now */
		TIM3_IRQHandler();
	}
	ENABLE_IRQ();
	return ret + tmr2;
}


uint64_t
monotonic_get_capture(void) {
	/* Get the previous PPS capture */
	uint64_t ret;
	DISABLE_IRQ();
	ret = mono_capture;
	mono_capture = 0;
	ENABLE_IRQ();
	return ret;
}


void
monotonic_sleep_until(uint64_t mono_when) {
	mono_when -= mono_when % MONO_PERIOD;
	DISABLE_IRQ();
	sleep_epoch = mono_when;
	ENABLE_IRQ();
	CoWaitForSingleFlag(sleep_flag, 0);
}
