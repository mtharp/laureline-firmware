/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "cmdline/cmdline.h"
#include "profile.h"
#include "OsTask.h"

#ifdef PROFILE_TASKS

uint64_t last_tick;

extern uint32_t _sheap;
void *_sbrk(intptr_t increment);

void
cli_cmd_profile(char *cmdline) {
    uint64_t task_counts[CFG_MAX_USER_TASKS+SYS_TASK_NUM];
    uint64_t count, pct, total = 0;
    void *heap;
    int i;

    DISABLE_IRQ();
    for(i = 0; i < CFG_MAX_USER_TASKS+SYS_TASK_NUM; i++) {
        task_counts[i] = count = TCBTbl[i].tick_count;
        total += count;
    }
    ENABLE_IRQ();

    cli_printf("#      Counts         %%    Name\r\n");
    for(i = 0; i < CFG_MAX_USER_TASKS+SYS_TASK_NUM; i++) {
        count = task_counts[i];
        pct = 10000 * count / total;
        cli_printf("%2d %08x%08x %3u.%02u%% %s\r\n",
                i,
                (uint32_t)(count >> 32),
                (uint32_t)count,
                (uint32_t)(pct / 100),
                (uint32_t)(pct % 100),
                TCBTbl[i].name);
    }

    heap = _sbrk(0);
    if (heap != (void*)-1) {
        cli_printf("Heap usage: %d bytes\r\n", heap - (void*)&_sheap);
    }
}
#endif


