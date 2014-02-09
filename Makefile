#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

PROJECT = laureline
# These are only used when not using a bootloader
HW_VERSION = 0
HSE_FREQ = 0

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
	-I$(BUILD) \
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
	-Wall -Wextra -Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-main \
	-Wno-address
LDLIBS = -lm -Wl,-u,_printf_float
DIST_OPTS = CFLAGS_EXTRA=-Werror

include $(TOP)/emk/rules.mk

discrim = $(VERSION),$(HW_VERSION),$(HSE_FREQ)
discrim_file = $(BUILD)/version.txt
ifneq ($(shell cat $(discrim_file) 2>/dev/null),$(discrim))
.PHONY: $(BUILD)/version.h
endif

$(BUILD)/version.h:
	echo '$(discrim)' > $(discrim_file)
	echo '#define VERSION    "$(VERSION)"' > $@
	echo '#define HW_VERSION $(HW_VERSION)' >> $@
	echo '#define HSE_FREQ   $(HSE_FREQ)' >> $@

src/main.c src/cmdline.c: $(BUILD)/version.h

dist:
	rm -rf dist
	$(MAKE) BUILD=dist/laureline-$(VERSION) $(DIST_OPTS)
	$(MAKE) -C bootloader BUILD=../dist/laureline-bootloader-hw7.1-osc25-$(VERSION) $(DIST_OPTS) HW_VERSION=0x0701 HSE_FREQ=25000000
	$(MAKE) -C bootloader BUILD=../dist/laureline-bootloader-hw7.1-osc26-$(VERSION) $(DIST_OPTS) HW_VERSION=0x0701 HSE_FREQ=26000000

.PHONY: dist
