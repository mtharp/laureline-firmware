/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "ppscapture.h"


/* High-order part of the monotonic timer */
static volatile uint64_t mono_epoch;
/* Last monotonic capture, or 0 if there are no pending captures */
static uint64_t mono_capture;

/* Reduce this to 1024 to make it easier to hit edge cases to test the input
 * capture code */
#define MONO_PERIOD 65536


void
ppscapture_start(void) {
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->CR1 = 0;
	TIM3->PSC = 0;
	TIM3->ARR = MONO_PERIOD - 1;
	/* input capture 1 receives the pulse-per-second */
	TIM3->CCMR1 = TIM_CCMR1_CC1S_0;
	/* output compare 4 fires halfway through each timer period */
	TIM3->CCMR2 = TIM_CCMR2_OC4PE | TIM_CCMR2_OC4M_0;
	TIM3->CCR4 = MONO_PERIOD / 2;
	TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC4E;
	/* interrupt on update and output compare. this means two interrupts per
	 * period, evenly spaced, during which the input capture can be sampled.
	 */
	TIM3->DIER = TIM_DIER_UIE | TIM_DIER_CC4IE;
	TIM3->SR = 0;

	NVIC_SetPriority(TIM3_IRQn, IRQ_PRIO_PPSCAPTURE);
	NVIC_EnableIRQ(TIM3_IRQn);
	TIM3->CR1 |= TIM_CR1_CEN;
}


void
TIM3_IRQHandler(void) {
	uint16_t sr, ccr;
	sr = TIM3->SR;
	TIM3->SR = 0;

	if (sr & TIM_SR_UIF) {
		mono_epoch += MONO_PERIOD;
	}

	if (sr & TIM_SR_CC1IF) {
		/* Twice per period, sample the input capture. This way it's
		 * unambiguous whether each capture is from the current epoch or the
		 * previous one, based on its value alone. */
		ccr = TIM3->CCR1;
		mono_capture = mono_epoch + ccr;
		if (sr & TIM_SR_UIF) {
			/* Start of the period */
			if (ccr > MONO_PERIOD / 4) {
				/* From the previous epoch */
				mono_capture -= MONO_PERIOD;
			}
			/* Else from the same epoch */
		} else if (sr & TIM_SR_CC4IF) {
			/* Halfway through the period */
			if (ccr > MONO_PERIOD * 3 / 4) {
				/* From the previous epoch */
				mono_capture -= MONO_PERIOD;
			}
			/* Else from the same epoch */
		}
	}
}


uint64_t
_monotonic_nowI(void) {
	/* Get value of monotonic clock */
	uint64_t ret;
	uint32_t save;
	uint16_t tmr1, tmr2;
	while (1) {
		tmr1 = TIM3->CNT;
		ret = mono_epoch;
		tmr2 = TIM3->CNT;
		if (tmr2 > tmr1) {
			break;
		}
		/* Timer rolled over while we were sampling. Wait for the rollover
		 * interrupt to finish then try again. */
		/* TODO: make sure this can't stall for a whole timer period if tickled
		 * the wrong way */
		while (mono_epoch == ret) {
			SAVE_ENABLE_IRQ(save);
			/* ISB might be required to ensure pending interrupts fire before
			 * they get disabled again */
			__ISB();
			RESTORE_DISABLE_IRQ(save);
		}
	}
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
