/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include <errno.h>
#include <stdint.h>


void
_exit(int rc) {
    while (1) {}
}


int
_kill(int pid, int signum) {
    return ESRCH;
}


int
_getpid(void) {
    return 1;
}


extern uint32_t _sheap;
extern uint32_t _eheap;
void *heap_top = &_sheap;

void *
_sbrk(intptr_t increment) {
    void *ret;
    if (heap_top + increment > (void*)&_eheap) {
        return (void*)-1;
    }
    ret = heap_top;
    heap_top += increment;
    return ret;
}
