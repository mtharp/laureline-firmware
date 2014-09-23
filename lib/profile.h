/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#ifndef _PROFILE_H
#define _PROFILE_H

#include <stdint.h>
#include "ppscapture.h"

#ifdef PROFILE_TASKS

extern uint64_t last_tick;

#define PROFILE_GET_TIMER monotonic_now()

#define PROFILE_EXIT_THREAD(var) { \
    U64 now = PROFILE_GET_TIMER; \
    var += now - last_tick; \
    last_tick = now; \
}


void cli_cmd_profile(char *cmdline);

#endif

#endif
