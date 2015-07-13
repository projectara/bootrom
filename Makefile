##
 # Copyright (c) 2015 Google Inc.
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met:
 # 1. Redistributions of source code must retain the above copyright notice,
 # this list of conditions and the following disclaimer.
 # 2. Redistributions in binary form must reproduce the above copyright notice,
 # this list of conditions and the following disclaimer in the documentation
 # and/or other materials provided with the distribution.
 # 3. Neither the name of the copyright holder nor the names of its
 # contributors may be used to endorse or promote products derived from this
 # software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 # AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 # THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 # OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 # WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 # OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 # ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ##

TOPDIR := ${shell pwd}

OUTROOT := build

#  VERBOSE==1:  Echo commands
#  VERBOSE!=1:  Do not echo commands
ifeq ($(VERBOSE),1)
export Q :=
else
export Q := @
endif

#
# "ES3 Bridge ASIC Boot ROM High Level Design" specifies 2 compile-time
# switches:
#    Production/Development: Assign UART to debug if Development
#    Normal/Simulation: If Simulation, bypass crypto and memory clearing so
#        that chip-level simulations run quicker.
#
XFLAGS =
#  _PRODUCTION==1:  Shipping version - prevent debug from using UART
#  _PRODUCTION!=1:  Development version - debug uses UART
ifeq ($(_PRODUCTION),1)
XCFLAGS += -D_PRODUCTION
XAFLAGS += -D_PRODUCTION
endif

#  _SIMULATION==1:  Simulate memory clearing and crypto for HW simulation
#  _SIMULATION!=1:  Use memory clearing and real crypto
ifeq ($(_SIMULATION),1)
XCFLAGS += -D_SIMULATION
XAFLAGS += -D_SIMULATION
endif


include $(TOPDIR)/.config

CONFIG_ARCH_CHIP  := $(patsubst "%",%,$(strip $(CONFIG_ARCH_CHIP)))
CHIP_DIR := $(TOPDIR)/chips/$(CONFIG_ARCH_CHIP)

ifdef CONFIG_ARCH_EXTRA
  ARCH_EXTRA_DIR = $(TOPDIR)/chips/$(CONFIG_ARCH_EXTRA)
endif

include $(CHIP_DIR)/Make.defs

CFLAGS += $(XCFLAGS)
AFLAGS += $(XAFLAGS)


_dummy := $(shell [ -d $(OUTROOT) ] || mkdir -p $(OUTROOT))
include $(TOPDIR)/Sources.mk
_dummy := $(foreach d,$(SRCDIRS), \
		  $(shell [ -d $(OUTROOT)/$(d) ] || mkdir -p $(OUTROOT)/$(d)))

ELF = $(OUTROOT)/bootrom
BIN = $(ELF).bin
HEX = $(ELF).hex

MANIFEST_OUTDIR = $(OUTROOT)/$(MANIFEST_SRCDIR)

CFLAGS += -DMANIFEST=$(MANIFEST) -DMANIFEST_HEADER="<$(MANIFEST).h>"
CFLAGS += -I$(MANIFEST_OUTDIR)

all: $(HEX)

$(MANIFEST_OUTDIR)/$(MANIFEST).h: $(MANIFEST_OUTDIR)/$(MANIFEST).mnfb
	@echo "Generating manifest data..."
	$(Q) xxd -i <$< >$@

$(MANIFEST_OUTDIR)/$(MANIFEST).mnfb: $(MANIFEST_OUTDIR)/$(MANIFEST)
	$(Q) manifesto $<

$(MANIFEST_OUTDIR)/$(MANIFEST): $(MANIFEST_SRCDIR)/$(MANIFEST)
	$(Q) cp $< $@

$(ELF): $(MANIFEST_OUTDIR)/$(MANIFEST).h $(AOBJS) $(COBJS)
	@ echo Linking $@
	$(Q) $(LD) -T $(LDSCRIPT) $(LINKFLAGS) -o $@ $(AOBJS) $(COBJS)

$(BIN): $(ELF)
	$(Q) $(OBJCOPY) $(OBJCOPYARGS) -O binary $< $@

# TBD: figure out the "version"
# also, the "--gpb" seems not needed because we are not differentiating
# the apbridge and gpbridge in the boot ROM
$(HEX): $(BIN)
	$(Q) bin2verilog --input $< --out $@ --version 1 --gpb

-include $(COBJS:.o=.d) $(AOBJS:.o=.d)

build/%.o: %.c
	@ echo Compiling $<
	$(Q) $(CC) $(CFLAGS) -MM -MT $@ -MF $(patsubst %.o,%.d,$@) -c $<
	$(Q) $(CC) $(CFLAGS) -o $@ -c $<

build/%.o: %.S
	@ echo Assembling $<
	$(Q) $(CC) $(AFLAGS) -MM -MT $@ -MF $(patsubst %.o,%.d,$@) -c $<
	$(Q) $(CC) $(AFLAGS) -o $@ -c $<

clean:
	$(Q) -rm -rf $(OUTROOT)

distclean: clean
	$(Q) -rm -rf .config
