#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#


BUILD ?= build
PROJECT ?= target
USE_LINK_GC = yes

C_SRCS = $(filter %.c,$(SRCS))
S_SRCS = $(filter %.s,$(SRCS))
OUT = $(BUILD)/$(PROJECT)

TRGT := arm-none-eabi-
CC   := $(TRGT)gcc
CPPC := $(TRGT)g++
LD   := $(TRGT)gcc -nostartfiles
CP   := $(TRGT)objcopy
AS   := $(TRGT)gcc -x assembler-with-cpp
OD   := $(TRGT)objdump
HEX  := $(CP) -O ihex
BIN  := $(CP) -O binary
GDB  := $(TRGT)gdb
PYTHON := python2

MCFLAGS = -mcpu=cortex-m3 -mthumb \
	-mno-thumb-interwork -DTHUMB_NO_INTERWORKING
ifdef DEBUG
CFLAGS_OPT = -O0
else
CFLAGS_OPT = -Os -fomit-frame-pointer
endif
CFLAGS += $(MCFLAGS) $(CFLAGS_OPT) $(CFLAGS_EXTRA)
CFLAGS += -ggdb3
CFLAGS += -MD -MP -MF $@.d

LDFLAGS = $(MCFLAGS) $(LDFLAGS_EXTRA)
LDFLAGS += -T$(LDSCRIPT) -Wl,-Map=$(OUT).map,--cref,--no-warn-mismatch
ifeq ($(USE_LINK_GC),yes)
CFLAGS += -ffunction-sections \
	  -fdata-sections \
	  -fno-common \
	  -falign-functions=16
LDFLAGS += -Wl,--gc-sections
endif

ASFLAGS = $(MCFLAGS)
CPFLAGS = -R boot_stub -R .boot_stub

GDB_INSTALL = $(GDB) --batch \
	-ex "tar ext $(BMP)" \
	-ex "mon connect_srst enable" \
	-ex "mon swdp_scan" \
	-ex "att 1" \
	-ex load \
	-ex kill

# Targets
all: $(OUT).elf $(OUT).hex $(OUT).bin $(OUT).lst

clean:
	rm -rf $(BUILD)

install: all
ifndef BMP
	$(error You must set BMP=/dev/ttyACMx)
endif
	$(GDB_INSTALL) $(OUT).elf

install-nostub: all
ifndef BMP
	$(error You must set BMP=/dev/ttyACMx)
endif
	$(CP) $(CPFLAGS) $(OUT).elf $(OUT).elf.nostub
	$(GDB_INSTALL) $(OUT).elf.nostub


# Rules
OBJS =
define c_template
OBJS += $(1)
$(1): $(2)
	@mkdir -p $(dir $(1))
	$$(CC) $$(CFLAGS) $$< -c -o $$@
endef
$(foreach src,$(C_SRCS),$(eval $(call c_template,$(BUILD)/$(subst ../,__/,$(src:.c=.o)),$(src))))
define s_template
OBJS += $(1)
$(1): $(2)
	@mkdir -p $(dir $(1))
	$$(AS) $$(ASFLAGS) $$< -c -o $$@
endef
$(foreach src,$(S_SRCS),$(eval $(call s_template,$(BUILD)/%(subst ../,__/,$(src:.s=.o)),$(src))))

$(OUT).elf: $(OBJS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

%.lst: %.elf
	$(OD) -S $< > $@

%.bin: %.elf
	$(BIN) $(CPFLAGS) $< $@

%.hex: %.elf
	$(HEX) $(CPFLAGS) $< $@


-include $(shell [ -d $(BUILD) ] && find $(BUILD) -name \*.d)
