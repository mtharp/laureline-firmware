/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "task.h"

#include "vtimer.h"
#include "gps/parser.h"
#include "util/parse.h"
#include <string.h>

static uint8_t rx_count, rx_cksum;
static enum {
    WAITING,
    COPYING,
    CHECKSUM1,
    CHECKSUM2
} rx_state;

/* In order of least to most preferred */
typedef enum {
    NONE,
    GPRMC,
    PGRMF,
    GPZDA
} stype_t;
static stype_t seen_type;
static TickType_t seen_time;


static uint8_t
use_sentence(stype_t type) {
    /* Ignore RMC if a ZDA was seen recently */
    if (type < seen_type &&
            (xTaskGetTickCount() - seen_time) < PARSER_TIMEOUT) {
        return 0;
    }
    seen_type = type;
    seen_time = xTaskGetTickCount();
    return 1;
}


uint8_t
nmea_feed(uint8_t val) {
    int16_t year;
    uint8_t hour, minute, second, day, month;
    char *ptr;
    if (val == '$') {
        rx_state = COPYING;
        rx_count = 0;
        rx_cksum = 0;
        return FEED_CONTINUE;
    }
    switch (rx_state) {
    case WAITING:
        return FEED_UNKNOWN;
    case COPYING:
        if (val == '*') {
            rx_state = CHECKSUM1;
            return FEED_CONTINUE;
        } else if (val == '\r' || val == '\n') {
            /* Sentence complete (no checksum) */
            break;
        } else {
            if (rx_count >= sizeof(pbuf) - 1) {
                rx_state = WAITING;
                return FEED_UNKNOWN;
            }
            pbuf[rx_count++] = val;
            rx_cksum ^= val;
            return FEED_CONTINUE;
        }
    case CHECKSUM1:
        rx_cksum ^= (parse_hex(val) << 4);
        rx_state = CHECKSUM2;
        return FEED_CONTINUE;
    case CHECKSUM2:
        rx_cksum ^= parse_hex(val);
        rx_state = WAITING;
        if (rx_cksum != 0) {
            return FEED_UNKNOWN;
        }
        /* Sentence complete (checksum valid) */
        break;
    }
    pbuf[rx_count] = 0;

    /* Parse the finished sentence */
    /* Type */
    if ((ptr = strtok_s((char*)pbuf, ',')) == NULL) {
        return FEED_COMPLETE;
    }

    if (strcmp(ptr, "GPZDA") == 0 && use_sentence(GPZDA)) {
        /* Time of day */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if (strlen(ptr) < 6) { return FEED_COMPLETE; }
        hour = atoi_2dig(&ptr[0]);
        minute = atoi_2dig(&ptr[2]);
        second = atoi_2dig(&ptr[4]);
        /* Day */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        day = atoi_decimal(ptr);
        /* Month */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        month = atoi_decimal(ptr);
        /* Year */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        year = atoi_decimal(ptr);
        /* Leap second support for NMEA will never be feasible :( */
        vtimer_set_utc(
                year,       /* year */
                month,      /* month */
                day,        /* day */
                hour,       /* hour */
                minute,     /* minute */
                second,     /* second */
                0);         /* leap */

    } else if (strcmp(ptr, "GPRMC") == 0 && use_sentence(GPRMC)) {
        /* Time of day */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if (strlen(ptr) < 6) { return FEED_COMPLETE; }
        hour = atoi_2dig(&ptr[0]);
        minute = atoi_2dig(&ptr[2]);
        second = atoi_2dig(&ptr[4]);
        /* Skip some stuff */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        /* Datestamp */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if (strlen(ptr) < 6) { return FEED_COMPLETE; }
        day = atoi_2dig(&ptr[0]);
        month = atoi_2dig(&ptr[2]);
        year = atoi_2dig(&ptr[4]);
        /* 2-digit years are evil, but at least this will work until 2113 */
        if (year >= 13) {
            year += 2000;
        } else {
            year += 2100;
        }
        vtimer_set_utc(
                year,       /* year */
                month,      /* month */
                day,        /* day */
                hour,       /* hour */
                minute,     /* minute */
                second,     /* second */
                0);         /* leap */

    } else if (strcmp(ptr, "PGRMF") == 0 && use_sentence(PGRMF)) {
        /* Skip some stuff */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        /* Datestamp */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if (strlen(ptr) < 6) { return FEED_COMPLETE; }
        day = atoi_2dig(&ptr[0]);
        month = atoi_2dig(&ptr[2]);
        year = atoi_2dig(&ptr[4]);
        if (year >= 13) {
            year += 2000;
        } else {
            year += 2100;
        }
        /* Time of day */
        if ((ptr = strtok_s(NULL, ',')) == NULL) { return FEED_COMPLETE; }
        if (strlen(ptr) < 6) { return FEED_COMPLETE; }
        hour = atoi_2dig(&ptr[0]);
        minute = atoi_2dig(&ptr[2]);
        second = atoi_2dig(&ptr[4]);
        vtimer_set_utc(
                year,       /* year */
                month,      /* month */
                day,        /* day */
                hour,       /* hour */
                minute,     /* minute */
                second,     /* second */
                0);         /* leap */
    }
    return FEED_COMPLETE;
}

