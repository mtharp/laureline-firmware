/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

#include "common.h"

#define INLINE inline
#define ROMCONST const
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END


#if !NO_SYS

typedef OS_EventID      sys_sem_t;
typedef OS_EventID      sys_mbox_t;
typedef OS_TID          sys_thread_t;
typedef OS_MutexID      sys_mutex_t;
typedef uint32_t        sys_prot_t;

#define SYS_MBOX_NULL   E_CREATE_FAILED
#define SYS_THREAD_NULL E_CREATE_FAILED
#define SYS_SEM_NULL    E_CREATE_FAILED

#endif /* NO_SYS */

#endif /* __SYS_ARCH_H__ */
