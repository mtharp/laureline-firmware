#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

PROJECT = laureline
BOARD = boards/v5

SRCS = $(wildcard src/*.c)
SRCS += $(wildcard src/gps/*.c)
SRCS += lib/coos_plat.c
SRCS += lib/hardfault.c
SRCS += lib/lwip/arch/sys_arch.c
SRCS += lib/profile.c
SRCS += lib/uptime.c
SRCS += lib/cmdline/core.c
SRCS += lib/cmdline/settings.c
SRCS += lib/stm32/dma.c
SRCS += lib/stm32/eth_mac.c
SRCS += lib/stm32/serial.c
SRCS += lib/util/parse.c
SRCS += lib/util/queue.c
SRCS += ports/core_cm3.c
SRCS += ports/crt0.c
SRCS += ports/stubs.c
SRCS += ports/vectors.c
SRCS += $(wildcard CoOS/kernel/*.c)
SRCS += CoOS/portable/arch.c
SRCS += CoOS/portable/GCC/port.c
SRCS += $(wildcard lwip/src/core/*.c)
SRCS += $(wildcard lwip/src/core/ipv4/*.c)
SRCS += $(wildcard lwip/src/core/api/*.c)
SRCS += $(wildcard lwip/src/api/*.c)
SRCS += lwip/src/netif/etharp.c
LDSCRIPT = ports/STM32F107xB.ld

PATH := /opt/tnt-20130915/bin:$(PATH)
INCLUDES = \
	-Isrc \
	-Isrc/conf \
	-Ilib \
	-Ilib/lwip \
	-Iports \
	-ICoOS/kernel \
	-ICoOS/portable \
	-Ilwip/src/include \
	-Ilwip/src/include/ipv4 \
	-Ilwip/src/include/ipv6
CFLAGS = \
	$(INCLUDES) \
	-Wall -Wextra -Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-main \
	-Wno-address
LDLIBS = -lm

#SRCS += src/fatfs/diskio.c fatfs/ff.c
#INCLUDES += -Isrc/fatfs -Ifatfs

include emk/rules.mk
