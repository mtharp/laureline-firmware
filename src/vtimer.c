/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "task.h"

#include "eeprom.h"
#include "epoch.h"
#include "init.h"
#include "logging.h"
#include "main.h"
#include "ntpns.h"
#include "pll.h"
#include "ppscapture.h"
#include "status.h"
#include "vtimer.h"
#include "stm32/iwdg.h"
#include <math.h>

unsigned sys_able;
TaskHandle_t thread_vtimer;

/* VT ticks this many NTP fractions per tick of monotonic time */
static double vt_rate;
/* Nominal rate for the current nominal system frequency */
static double vt_rate_nominal;
/* VT value at the last timer update */
static uint64_t vt_last;
/* Monotonic value at the last timer update */
static uint64_t mono_last;
/* Saved corrections from GPS */
static uint32_t utc_next;
static float quant_corr, quant_corr_deferred;
/* Saved loopstats report for SNMP */
int32_t loopstats_values[LOOPSTATS_VALUES];

static void pll_thread(void *p);
static uint64_t vtimer_getI(uint64_t mono_next);
static void vtimer_updateI(void);
static double vtimer_get_frac_delta(uint64_t mono_capture);
static void vtimer_step(double dd);


void
vtimer_start(void) {
    init_pllmath();
    pll_reset();
    quant_corr = quant_corr_deferred = 0.0f;
    vt_rate_nominal = NTP_TO_FLOAT / system_frequency;
    ASSERT(xTaskCreate(pll_thread, "vtimer", VTIMER_STACK_SIZE, NULL,
                THREAD_PRIO_VTIMER, &thread_vtimer));
}


static void
pll_thread(void *p) {
    static TickType_t last_pps;
    static uint64_t tmp, last;
    static int64_t tmps;
    static double delta, ppb;
    static uint8_t desync = 0;
    static uint16_t old_status;
    static uint16_t next_report = 0;
    static float ojit = 0, fjit = 0, ppb_last = 0, ppb_delta;
    while (1) {
        tmp = monotonic_get_capture();
        old_status = status_flags;
        if (tmp && !(cfg.flags & FLAG_HOLDOVER_TEST)) {
            delta = vtimer_get_frac_delta(tmp);
            if (status_flags & STATUS_PLL_OK) {
                if (pll_state.st < 3) {
                    log_write(LOG_WARNING, "vtimer", "PLL is not locked!");
                    clear_status(STATUS_PLL_OK);
                }
            } else {
                if (pll_state.st >= 3
                        && delta > -SETTLED_THRESH
                        && delta < SETTLED_THRESH) {
                    log_write(LOG_NOTICE, "vtimer", "PLL has become locked");
                    set_status(STATUS_PLL_OK);
                }
            }
            if (delta < -STEP_THRESH || delta > STEP_THRESH) {
                if (++desync >= 5) {
                    vtimer_step(-delta);
                    init_pllmath();
                    pll_reset();
                    desync = 0;
                    log_write(LOG_NOTICE, "vtimer",
                            "step(PPS) %d us", (int)(-delta * 1e6));
                }
            } else {
                desync = 0;
            }
            if (((xTaskGetTickCount() - last_pps) < pdMS_TO_TICKS(1100)) && !(status_flags & STATUS_PPS_OK)) {
                log_write(LOG_NOTICE, "vtimer", "PPS detected");
                set_status(STATUS_PPS_OK);
            }
            last_pps = xTaskGetTickCount();
            ppb = pll_math(delta);
        } else {
            if (((xTaskGetTickCount() - last_pps) >= pdMS_TO_TICKS(5000))
                    && (status_flags & STATUS_PPS_OK)) {
                log_write(LOG_WARNING, "vtimer", "PPS is not valid!");
                clear_status(STATUS_PPS_OK);
            }
            if (((xTaskGetTickCount() - last_pps) >= (pdMS_TO_TICKS(1000) * cfg.holdover))
                    && (status_flags & STATUS_PLL_OK)) {
                log_write(LOG_ERR, "vtimer", "PLL lock timed out due to lack of PPS");
                clear_status(STATUS_PLL_OK);
            }
            if (tmp && next_report == 0) {
                /* Holdover test mode */
                delta = vtimer_get_frac_delta(tmp);
                log_write(LOG_WARNING, "vtimer", "HOLDOVER TEST!  current offset: %d ns", (int)(delta*1e9));
            }
            ppb = pll_poll();
        }
        kern_freq(ppb);

        /* Update vtimer */
        DISABLE_IRQ();
        vtimer_updateI();
        tmps = 0;
        last = vt_last;
        if (utc_next != 0) {
            tmps = (int64_t)utc_next - (int64_t)(last >> 32);
            utc_next = 0;
            status_flags |= STATUS_TOD_OK;
        }
        if (watchdog_main && watchdog_net) {
            iwdg_clear();
            watchdog_main--;
            watchdog_net--;
        }
        ENABLE_IRQ();

        if (tmps != 0) {
            vtimer_step(tmps);
            if (tmps >> 31) {
                /* Too big for signed int32 */
                log_write(LOG_NOTICE, "vtimer", "step(UTC) %d sec", (uint32_t)tmps);
            } else {
                log_write(LOG_NOTICE, "vtimer", "step(UTC) %d sec", (int32_t)tmps);
            }
            DISABLE_IRQ();
            vtimer_updateI();
            last = vt_last;
            ENABLE_IRQ();
        }
        if (!(old_status & STATUS_TOD_OK) && (status_flags & STATUS_TOD_OK)) {
            log_write(LOG_NOTICE, "vtimer", "Time of day is correct");
        }

        /* Update loopstats */
        ppb_delta = ppb - ppb_last;
        ppb_last = ppb;
        if (pll_state.j) {
            ojit += (pll_state.dy*pll_state.dy - ojit) / pll_state.j;
            fjit += (ppb_delta*ppb_delta - fjit) / pll_state.j;
        }
        {
            float fjitter = 1e12*sqrtf(fjit);
            if (fjitter < INT32_MIN || fjitter > INT32_MAX) {
                fjitter = 0.0f;
            }
            loopstats_values[0] = (int)(1e9*pll_state.my);  /* timeOffset */
            loopstats_values[1] = (int)(1e9*ppb);           /* frequencyOffset */
            loopstats_values[2] = (int)(1e9*sqrtf(ojit));   /* timeJitter */
            loopstats_values[3] = (int)fjitter;             /* frequencyJitter */
            loopstats_values[4] = (int)pll_state.j;         /* loopTimeConstant */
            loopstats_values[5] = pll_state.st;             /* pllState */
        }
        if (next_report == 0) {
            next_report = cfg.loopstats_interval - 1;
            log_write(LOG_INFO, "loopstats", "off:%dns freq:%dppb jit:%dns fjit:%dppt looptc:%ds state:%d flags:%sPPS,%sToD,%sPLL,%sQUANT",
                    loopstats_values[0], loopstats_values[1],
                    loopstats_values[2], loopstats_values[3],
                    loopstats_values[4], loopstats_values[5],
                    (status_flags & STATUS_PPS_OK) ? "" : "!",
                    (status_flags & STATUS_TOD_OK) ? "" : "!",
                    (status_flags & STATUS_PLL_OK) ? "" : "!",
                    (status_flags & STATUS_USED_QUANT) ? "" : "!");

        } else {
            next_report--;
        }

        /* Wait until top of second, blink LED, then run the PLL again 100ms
         * before the next second */
        tmp = last & NTP_MASK_SECONDS;
        tmp += NTP_SECOND; /* top of next second in vtimer time */
        vtimer_sleep_until(tmp);
        if (~status_flags & STATUS_VALID) {
            GPIO_ON(LED3);
            if (status_flags & STATUS_PPS_OK) {
                /* top: solid orange */
                GPIO_ON(LED4);
            } else {
                /* top: solid red */
                GPIO_OFF(LED4);
            }
        } else {
            /* top: solid green */
            GPIO_OFF(LED3);
            GPIO_ON(LED4);
        }

        if ((status_flags & STATUS_SETTLED) == STATUS_SETTLED) {
            /* bottom: flash green */
            GPIO_ON(LED2);
        } else if (status_flags & STATUS_SETTLED) {
            /* bottom: flash red (PPS but no lock, or holdover but no PPS) */
            GPIO_ON(LED1);
        } else {
            /* bottom: off (no PPS or holdover) */
        }
        tmp += PPS_BLINK_TIME * NTP_SECOND;
        vtimer_sleep_until(tmp);
        GPIO_OFF(LED1);
        GPIO_OFF(LED2);
        tmp += (PLL_SUB_TIME - PPS_BLINK_TIME) * NTP_SECOND;
        vtimer_sleep_until(tmp);
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
    double frac;
    float corr;
    DISABLE_IRQ();
    vt_capture = vtimer_getI(mono_capture);
    corr = quant_corr;
    quant_corr= quant_corr_deferred;
    quant_corr_deferred = 0.0f;
    if (corr != 0.0f) {
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
    if (f > 500e-6) {
        f = 500e-6;
    } else if (f < -500e-6) {
        f = -500e-6;
    }
    next_rate = vt_rate_nominal * (1.0 + f);
    DISABLE_IRQ();
    vt_rate = next_rate;
    ENABLE_IRQ();
}


static void
vtimer_step(double dd) {
    /* Apply a step change to the vtimer */
    DISABLE_IRQ();
    vt_last += (int64_t)((double)dd * NTP_TO_FLOAT);
    vtimer_updateI();
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
    ntp_seconds = datetime_to_epoch(year, month, day, hour, minute, second);
    DISABLE_IRQ();
    utc_next = ntp_seconds;
    ENABLE_IRQ();
}


void
vtimer_set_gps(uint16_t wkn, uint32_t tow, int16_t leap, uint8_t leap_valid) {
    uint32_t ntp_seconds = gps_to_epoch(wkn, tow);
    if (!(cfg.flags & FLAG_TIMESCALE_GPS)) {
        if (!leap_valid) {
            return;
        }
        ntp_seconds -= leap;
    }
    DISABLE_IRQ();
    utc_next = ntp_seconds;
    ENABLE_IRQ();
}


void
vtimer_set_correction(float corr, quant_leadlag_t leadlag) {
    DISABLE_IRQ();
    if (leadlag == LEADING) {
        quant_corr_deferred = corr;
    } else {
        quant_corr = corr;
        quant_corr_deferred = 0.0f;
    }
    ENABLE_IRQ();
}


void
vtimer_sleep_until(uint64_t vt_when) {
    uint64_t mono_when;
    DISABLE_IRQ();
    mono_when = mono_last + (uint64_t)((float)(vt_when - vt_last) / vt_rate);
    ENABLE_IRQ();
    monotonic_sleep_until(mono_when);
}
