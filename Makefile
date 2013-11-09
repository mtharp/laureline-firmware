#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

PROJECT = laureline

TOP = .
SRCS = $(wildcard src/*.c)
SRCS += $(wildcard src/gps/*.c)
SRCS += $(TOP)/lib/coos_plat.c
SRCS += $(TOP)/lib/hardfault.c
SRCS += $(TOP)/lib/info_table.c
SRCS += $(TOP)/lib/lwip/arch/sys_arch.c
SRCS += $(TOP)/lib/profile.c
SRCS += $(TOP)/lib/uptime.c
SRCS += $(TOP)/lib/cmdline/core.c
SRCS += $(TOP)/lib/cmdline/settings.c
SRCS += $(TOP)/lib/stm32/dma.c
SRCS += $(TOP)/lib/stm32/eth_mac.c
SRCS += $(TOP)/lib/stm32/i2c.c
SRCS += $(TOP)/lib/stm32/iwdg.c
SRCS += $(TOP)/lib/stm32/serial.c
SRCS += $(TOP)/lib/util/parse.c
SRCS += $(TOP)/lib/util/queue.c
SRCS += $(TOP)/ports/core_cm3.c
SRCS += $(TOP)/ports/crt0.c
SRCS += $(TOP)/ports/stubs.c
SRCS += $(TOP)/ports/vectors.c
SRCS += $(wildcard $(TOP)/CoOS/kernel/*.c)
SRCS += $(TOP)/CoOS/portable/arch.c
SRCS += $(TOP)/CoOS/portable/GCC/port.c
SRCS += $(wildcard $(TOP)/lwip/src/core/*.c)
SRCS += $(wildcard $(TOP)/lwip/src/core/ipv4/*.c)
SRCS += $(wildcard $(TOP)/lwip/src/core/api/*.c)
SRCS += $(wildcard $(TOP)/lwip/src/api/*.c)
SRCS += $(TOP)/lwip/src/netif/etharp.c
LDSCRIPT = ports/STM32F107xB.ld

PATH := /opt/tnt-20130915/bin:$(PATH)
VERSION ?= $(shell $(TOP)/util/git_version.sh)
INCLUDES = \
	-Isrc \
	-Isrc/conf \
	-I$(TOP)/lib \
	-I$(TOP)/lib/lwip \
	-I$(TOP)/ports \
	-I$(TOP)/CoOS/kernel \
	-I$(TOP)/CoOS/portable \
	-I$(TOP)/lwip/src/include \
	-I$(TOP)/lwip/src/include/ipv4 \
	-I$(TOP)/lwip/src/include/ipv6
CFLAGS = \
	$(INCLUDES) \
	-DVERSION='"$(VERSION)"' \
	-Wall -Wextra -Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-main \
	-Wno-address
LDLIBS = -lm

include $(TOP)/emk/rules.mk
