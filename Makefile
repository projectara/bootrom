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

ifeq ($(BOOT_STAGE),)
# default build for the boot ROM, which is the first stage in booting sequence
BOOT_STAGE = 1
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
else
# Non-production builds: testing switches allowed
include $(TOPDIR)/testing.mk
endif



ifeq ($(BUILD_FOR_GBBOOT_SERVER),1)
XCFLAGS += -DBUILD_FOR_GBBOOT_SERVER
XAFLAGS += -DBUILD_FOR_GBBOOT_SERVER
CONFIG_DEBUG = y
endif


ifeq ($(BUILD_FOR_SIGN_VERIFY),1)
XCFLAGS += -DBUILD_FOR_SIGN_VERIFY
XAFLAGS += -DBUILD_FOR_SIGN_VERIFY
CONFIG_DEBUG = y
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
HEXAP = $(OUTROOT)/bromcAP.dat
HEXGP = $(OUTROOT)/bromcGP.dat

MANIFEST_OUTDIR = $(OUTROOT)/$(MANIFEST_SRCDIR)

CFLAGS += -DMANIFEST=$(MANIFEST) -DMANIFEST_HEADER="<$(MANIFEST).h>"
CFLAGS += -I$(MANIFEST_OUTDIR)

CFLAGS += -DBOOT_STAGE=$(BOOT_STAGE)
AFLAGS += -DBOOT_STAGE=$(BOOT_STAGE)

COBJS += $(MANIFEST_OUTDIR)/manifest.o $(MANIFEST_OUTDIR)/public_keys.o

all: $(HEXAP) $(HEXGP)

$(MANIFEST_OUTDIR)/%.o: $(MANIFEST_OUTDIR)/%.c
	$(Q) $(CC) $(CFLAGS) -o $@ -c $<

$(MANIFEST_OUTDIR)/public_keys.c: $(PUBLIC_KEYS_FILE)
	@ echo "Use "$<" as the public keys file"
	$(Q) cp $< $@

$(MANIFEST_OUTDIR)/manifest.c: $(MANIFEST_OUTDIR)/manifest.mnfb
	@echo "Generating manifest data..."
	$(Q) xxd -i $< >$@

$(MANIFEST_OUTDIR)/manifest.mnfb: $(MANIFEST_OUTDIR)/manifest
	$(Q) manifesto $<

$(MANIFEST_OUTDIR)/manifest: $(MANIFEST_SRCDIR)/$(MANIFEST)
	$(Q) cp $< $@

$(ELF): $(AOBJS) $(COBJS)
	@ echo Linking $@
	$(Q) $(LD) -T $(LDSCRIPT) $(LINKFLAGS) -o $@ $(AOBJS) $(COBJS) $(EXTRALIBS)

$(BIN): $(ELF)
	$(Q) $(OBJCOPY) $(OBJCOPYARGS) -O binary $< $@

# TBD: figure out the "version"
HEXVER=0
$(HEXGP): $(BIN)
	$(Q) tools/bin2verilog --input $< --out $@ \
		--version $(HEXVER) --gpb --size 0x4000

$(HEXAP): $(BIN)
	$(Q) tools/bin2verilog --input $< --out $@ \
		--version $(HEXVER) --apb --size 0x4000

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

second_stage:
	@ echo "Building for second stage boot loader"
	$(Q) VERBOSE=$(VERBOSE) BOOT_STAGE=2 make --no-print-directory

third_stage:
	@ echo "Building for third stage boot firmware"
	$(Q) VERBOSE=$(VERBOSE) BOOT_STAGE=3 make --no-print-directory

sign_verify:
	@ echo "Building special sign-verify image"
	$(Q) VERBOSE=$(VERBOSE) BUILD_FOR_SIGN_VERIFY=1 make --no-print-directory

gbboot_server:
	@ echo "Building server for downloading FW over UniPro"
	$(Q) VERBOSE=$(VERBOSE) BUILD_FOR_GBBOOT_SERVER=1 make --no-print-directory
