#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

PROJECT = bootloader
HW_VERSION = 0
FLASH_SIZE = 256k

TOP = ..
SRCS = $(wildcard src/*.c)
SRCS += $(TOP)/src/board.c
SRCS += $(TOP)/src/init.c
SRCS += $(TOP)/fatfs/ff.c
SRCS += $(TOP)/lib/coos_plat.c
SRCS += $(TOP)/lib/crc7.c
SRCS += $(TOP)/lib/fatfs/mmc_diskio.c
SRCS += $(TOP)/lib/hardfault.c
SRCS += $(TOP)/lib/ihex.c
SRCS += $(TOP)/lib/stm32/dma.c
SRCS += $(TOP)/lib/stm32/flash.c
SRCS += $(TOP)/lib/stm32/mmc.c
SRCS += $(TOP)/lib/stm32/serial.c
SRCS += $(TOP)/lib/stm32/spi.c
SRCS += $(TOP)/lib/util/queue.c
SRCS += $(TOP)/ports/core_cm3.c
SRCS += $(TOP)/ports/crt0.c
SRCS += $(TOP)/ports/stubs.c
SRCS += $(TOP)/ports/vectors.c
SRCS += $(wildcard $(TOP)/CoOS/kernel/*.c)
SRCS += $(TOP)/CoOS/portable/arch.c
SRCS += $(TOP)/CoOS/portable/GCC/port.c
LDSCRIPT = ports/STM32F107xB.ld

PATH := /opt/tnt-20130915/bin:$(PATH)
INCLUDES = \
	-I$(BUILD) \
	-Isrc \
	-I$(TOP)/src \
	-I$(TOP)/lib \
	-I$(TOP)/lib/lwip \
	-I$(TOP)/ports \
	-I$(TOP)/CoOS/kernel \
	-I$(TOP)/CoOS/portable \
	-I$(TOP)/fatfs

VERSION ?= $(shell $(TOP)/util/git_version.sh)
CFLAGS = \
	$(INCLUDES) \
	-DBOOTLOADER \
	-Wall -Wextra -Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-main \
	-Wno-address

include $(TOP)/emk/rules.mk

LD := $(PYTHON) $(TOP)/util/resize_ld.py $(FLASH_SIZE) $(LD)

ifeq ($(HW_VERSION),0)
$(error Must set HW_VERSION to e.g. 0x0701 for 7.1)
else

ifneq ($(shell cat $(BUILD)/version.txt 2>/dev/null),$(VERSION))
.PHONY: $(BUILD)/version.h
endif
ifneq ($(shell cat $(BUILD)/hwversion.txt 2>/dev/null),$(HW_VERSION))
.PHONY: $(BUILD)/version.h
endif

$(BUILD)/version.h:
	echo $(VERSION) > $(BUILD)/version.txt
	echo $(HW_VERSION) > $(BUILD)/hwversion.txt
	echo '#define VERSION "$(VERSION)"' > $@
	echo '#define HW_VERSION $(HW_VERSION)' >> $@

endif

src/main.c src/cmdline.c: $(BUILD)/version.h
