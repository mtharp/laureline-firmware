/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "cmdline.h"
#include "epoch.h"
#include "init.h"
#include "ntpns.h"
#include "pll.h"
#include "ppscapture.h"
#include "status.h"
#include "vtimer.h"

#define VTIMER_STACK 512
OS_STK vtimer_stack[VTIMER_STACK];
OS_TID vtimer_tid;

unsigned sys_able;

/* VT ticks this many NTP fractions per tick of monotonic time */
static double vt_rate;
/* This many systicks pass per second of NTP time */
static float vt_rate_inv_sys;
/* Nominal rates for the current nominal system frequency */
static double vt_rate_nominal;
static float vt_rate_inv_sys_nominal;
/* VT value at the last timer update */
static uint64_t vt_last;
/* Monotonic value at the last timer update */
static uint64_t mono_last;
/* Saved corrections from GPS */
static uint32_t utc_next;
static double quant_corr;

static void pll_thread(void *p);
static uint64_t vtimer_getI(uint64_t mono_next);
static void vtimer_updateI(void);
static double vtimer_get_frac_delta(uint64_t mono_capture);
static void vtimer_step(double dd);


void
vtimer_start(void) {
	init_pllmath();
	pll_reset();
	vt_rate_nominal = NTP_TO_FLOAT / system_frequency;
	vt_rate_inv_sys_nominal = CFG_SYSTICK_FREQ / NTP_TO_FLOAT;
	vtimer_tid = CoCreateTask(pll_thread, NULL, THREAD_PRIO_VTIMER,
			&vtimer_stack[VTIMER_STACK-1], VTIMER_STACK, "vtimer");
	ASSERT(vtimer_tid != E_CREATE_FAIL);
}


static void
pll_thread(void *p) {
	static uint64_t last_pps;
	static uint64_t tmp, last;
	static int64_t tmps;
	static double delta;
	static uint8_t desync;
	desync = 0;
	while (1) {
		tmp = monotonic_get_capture();
		if (tmp) {
			delta = vtimer_get_frac_delta(tmp);
			if (delta < -SETTLED_THRESH || delta > SETTLED_THRESH) {
				clear_status(STATUS_PLL_OK);
			} else {
				set_status(STATUS_PLL_OK);
			}
			if (delta < -STEP_THRESH || delta > STEP_THRESH) {
				if (++desync >= 5) {
					idle_printf("step(PPS) %d us\r\n", (int32_t)(-delta * 1e6));
					vtimer_step(-delta);
					init_pllmath();
					pll_reset();
					desync = 0;
				}
			} else {
				desync = 0;
			}
			if ((CoGetOSTime() - last_pps) < MS2ST(1100)) {
				set_status(STATUS_PPS_OK);
			}
			last_pps = CoGetOSTime();
			idle_printf("pps %d ns  ", (int32_t)(delta*1e9));
			delta = pll_math(delta);
		} else {
			if ((CoGetOSTime() - last_pps) >= S2ST(5)) {
				clear_status(STATUS_PPS_OK);
			}
			idle_printf("NO PPS!  ");
			delta = pll_poll();
		}
		idle_printf("freq %d ppb\r\n", (int32_t)(delta*1e9));
		kern_freq(delta);

		/* Update vtimer */
		DISABLE_IRQ();
		vtimer_updateI();
		tmps = 0;
		last = vt_last;
		if (utc_next != 0) {
			tmps = (int64_t)utc_next - (int64_t)(last >> 32);
			utc_next = 0;
		}
		ENABLE_IRQ();

		if (tmps != 0) {
			idle_printf("step(UTC) %d us\r\n", (int32_t)(tmps * 1e6));
			vtimer_step(tmps);
		}

		/* Wait until top of second, blink LED, then run the PLL again 100ms
		 * before the next second */
		tmp = last & NTP_MASK_SECONDS;
		tmp += NTP_SECOND; /* top of next second in vtimer time */
		delta = (tmp - last) * vt_rate_inv_sys; /* in systick time */
		CoTickDelay(delta);
		if (isSettled()) {
			/* bottom: flash green */
			GPIO_ON(LED2);
			/* top: solid green (TODO: check GPS health) */
			GPIO_OFF(LED3);
			GPIO_ON(LED4);
		} else if (status_flags & STATUS_PPS_OK) {
			/* bottom: flash red */
			GPIO_ON(LED1);
			/* top: solid green (TODO: check GPS health) */
			GPIO_OFF(LED3);
			GPIO_ON(LED4);
		} else {
			/* bottom: off */
			/* top: solid red */
			GPIO_ON(LED3);
			GPIO_OFF(LED4);
		}
		CoTickDelay(PPS_BLINK_TIME * NTP_SECOND * vt_rate_inv_sys);
		GPIO_OFF(LED1);
		GPIO_OFF(LED2);
		CoTickDelay((PLL_SUB_TIME - PPS_BLINK_TIME) * NTP_SECOND * vt_rate_inv_sys);
	}
}


static uint64_t
vtimer_getI(uint64_t mono_next) {
	/* Returns the current vtimer time given the current monotonic time */
	return vt_last + (int64_t)((double)(mono_next - mono_last) * vt_rate);
}

static void
vtimer_updateI(void) {
	/* Advance the vtimer's "base". Must be called once per second to maintain
	 * accuracy. */
	uint64_t mono_next, vt_next;
	mono_next = monotonic_now();
	vt_next = vtimer_getI(mono_next);
	vt_last = vt_next;
	mono_last = mono_next;
}


static double
vtimer_get_frac_delta(uint64_t mono_capture) {
	/* Convert a PPS capture in monotonic time to vtimer, but only the
	 * fractional second part is kept. */
	uint64_t vt_capture;
	double frac, corr;
	DISABLE_IRQ();
	vt_capture = vtimer_getI(mono_capture);
	corr = quant_corr;
	quant_corr = 0.0;
	if (corr != 0.0) {
		status_flags |= STATUS_USED_QUANT;
	} else {
		status_flags &= ~STATUS_USED_QUANT;
	}
	ENABLE_IRQ();
	vt_capture &= NTP_MASK_FRAC;
	frac = (double)(uint32_t)vt_capture / NTP_TO_FLOAT;
	frac += corr;
	if (frac > 0.5f) {
		frac -= 1.0f;
	}
	return frac;
}


void
kern_freq(double f) {
	/* Change the rate at which the vtimer ticks */
	double next_rate;
	float next_inv;
	if (f > 500e-6) {
		f = 500e-6;
	} else if (f < -500e-6) {
		f = -500e-6;
	}
	next_rate = vt_rate_nominal * (1.0 + f);
	next_inv = vt_rate_inv_sys_nominal / (1.0 + f);
	DISABLE_IRQ();
	vt_rate = next_rate;
	vt_rate_inv_sys = next_inv;
	ENABLE_IRQ();
}


static void
vtimer_step(double dd) {
	/* Apply a step change to the vtimer */
	DISABLE_IRQ();
	vt_last += (int64_t)((double)dd * NTP_TO_FLOAT);
	ENABLE_IRQ();
}


uint64_t
vtimer_now(void) {
	uint64_t tmp;
	DISABLE_IRQ();
	tmp = monotonic_now();
	tmp = vtimer_getI(tmp);
	ENABLE_IRQ();
	return tmp;
}


void
vtimer_set_utc(uint16_t year, uint8_t month, uint8_t day,
		uint8_t hour, uint8_t minute, uint8_t second, uint8_t leap) {
	uint32_t ntp_seconds;
	ntp_seconds = datetime_to_ntp(year, month, day, hour, minute, second);
	DISABLE_IRQ();
	utc_next = ntp_seconds;
	status_flags |= STATUS_TOD_OK;
	ENABLE_IRQ();
}



void
vtimer_set_correction(double corr) {
	DISABLE_IRQ();
	quant_corr = corr;
	ENABLE_IRQ();
}
