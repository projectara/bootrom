TOPDIR := ${shell pwd}

# Include build configuration
include $(MCL_CONFIG_DIR)/defconfig
include $(MCL_CONFIG_DIR)/config.mk

# Directory for .o files and final executibles
_dummy := $(shell [ -d $(OUTBUILD) ] || mkdir -p $(OUTBUILD))
# Director for Libraries and API .o files
_dummy := $(shell [ -d $(OUTBIN) ] || mkdir -p $(OUTBIN))

#  V==1: Echo commands
#  V!=1: Do not echo commands
ifeq ($(V),1)
export Q :=
else
export Q := @
endif

# include files directory
INCLUDEDIR=-I ./include

# Default decorator value. This changes the name of the 
# functions that are dependent on the curve.
DREC := EC
DRRSA := RSA

# Library that includes ECC code
LIBMCLCURVE=$(OUTBIN)/libmclcurve$(DREC).a
# Library that inccludes code that does not depend on curve choice
LIBMCLCORE=$(OUTBIN)/libmclcore.a

ifeq ($(CONFIG_ARM),y)
  CC := $(CROSS_COMPILE)gcc
  LD = $(CROSS_COMPILE)ld
  AR = $(CROSS_COMPILE)ar
  AS = $(CROSS_COMPILE)as
  NM = $(CROSS_COMPILE)nm
  STRIP = $(CROSS_COMPILE)strip --strip-all
  SIZE = $(CROSS_COMPILE)size
  OBJCOPY = $(CROSS_COMPILE)objcopy
  OBJDUMP = $(CROSS_COMPILE)objdump

  LD=$(CC)

  CFLAGS=-Wno-address -Os -fdata-sections -ffunction-sections \
         -ffreestanding -Wall -fno-strict-aliasing -fno-common $(ARCH_FLAGS) 

  LDFLAGS=-Wl,--start-group   $(AWS_SDK)bin/libs/libfreertos.a \
          $(AWS_SDK)wmsdk/libs/libwmsdk.a $(LIBMCLCURVE) $(LIBMCLCORE)  -Wl,--end-group \
          -T $(AWS_SDK)build/toolchains/GNU/mw300.ld -nostartfiles -Xlinker \
          -M -Xlinker --cref -Xlinker --gc-sections -Xlinker --defsym=_rom_data=64 $(ARCH_FLAGS) 

  INCLUDEDIR+=-I$(AWS_SDK)wmsdk/src/incl/libc \
              -I$(AWS_SDK)wmsdk/src/incl/sdk \
              -I$(AWS_SDK)wmsdk/src/incl/sdk/drivers \
              -I$(AWS_SDK)wmsdk/src/incl/sdk/drivers/mw300 \
              -I$(AWS_SDK)wmsdk/src/incl/sdk/drivers/mw300/regs \
              -I$(AWS_SDK)wmsdk/src/incl/platform/os/freertos \
              -I$(AWS_SDK)wmsdk/external/freertos/Source/include \
              -I$(AWS_SDK)wmsdk/external/freertos/Source/portable/GCC/ARM_CM3 \
              -I$(AWS_SDK)wmsdk/external/freertos 
else
  CC=gcc
  LD=$(CC)
  CFLAGS=-std=c99 -g -Wall 
  LDFLAGS=-Wl,--start-group $(LIBMCLCURVE) $(LIBMCLCORE) -Wl,--end-group 
  STRIP=strip --strip-all
  SIZE=size
endif

ifeq ($(CONFIG_ARM),y)
  CFLAGS+=-D MCL_BUILD_ARM
endif 

ifeq ($(CONFIG_TEST),y)
  CFLAGS+=-D MCL_BUILD_TEST
endif 

CFLAGS+= -D MCL_CHUNK=$(MCL_CHUNK) -D MCL_CHOICE=$(MCL_CHOICE) \
         -D MCL_CURVETYPE=$(MCL_CURVETYPE) -D MCL_FFLEN=$(MCL_FFLEN) 

# Decorator for functions
ifeq ($(CONFIG_DECORATOR),y)
  include $(TOPDIR)/Decorator.mk
  CFLAGS+= $(DRFLAGS)
endif

# library src files directory 
LIB_DIR=$(TOPDIR)/src/lib

# benchmark src files directory
BENCH_DIR=$(TOPDIR)/src/benchmark

# test src files directory
TEST_DIR=$(TOPDIR)/src/tests

# Crypto library source
include $(TOPDIR)/Sources.mk
LIBCURVE_OBJS  := $(LIBCURVE_SRC:$(LIB_DIR)/%.c=$(OUTBUILD)/%.o)
LIBCORE_OBJS := $(LIBCORE_SRC:$(LIB_DIR)/%.c=$(OUTBUILD)/%.o)
STEST_OBJS  := $(STEST_SRC:$(TEST_DIR)/%.c=$(OUTBUILD)/%.o)
STEST_EXE  := $(STEST_SRC:$(TEST_DIR)/%.c=$(OUTBUILD)/%)
BENCH_OBJS  := $(BENCH_SRC:$(BENCH_DIR)/%.c=$(OUTBUILD)/%.o)
BENCH_EXE  := $(BENCH_SRC:$(BENCH_DIR)/%.c=$(OUTBUILD)/%)
TEST_OBJS  := $(TEST_SRC:$(TEST_DIR)/%.c=$(OUTBUILD)/%.o)
TEST_EXE  := $(TEST_SRC:$(TEST_DIR)/%.c=$(OUTBUILD)/%)

# Assign targets
TARGET := $(LIBMCLCURVE) $(LIBMCLCORE) 
ifeq ($(CONFIG_TEST),y)

TARGET += $(STEST_EXE) $(BENCH_EXE)
# Do not build units tests for embedded.
ifeq ($(CONFIG_ARM),n)
  TARGET += $(TEST_EXE)
endif 

endif 

MCL_RSA_OBJS := $(OUTBIN)/mcl_rsa$(DRRSA).o
MCL_ECDH_OBJS := $(OUTBIN)/mcl_ecdh$(DREC).o
MCL_UTILS_OBJS := $(OUTBUILD)/mcl_utils.o
LINK_OBJS := $(MCL_RSA_OBJS) $(MCL_ECDH_OBJS) $(MCL_UTILS_OBJS)
ifeq ($(CONFIG_ARM),y)
MW302_OBJS := $(OUTBUILD)/mw302_rd.o
LINK_OBJS += $(MW302_OBJS)
endif

all: $(TARGET)

$(LIBMCLCURVE): $(LIBCURVE_OBJS)
	$(Q)$(AR) $(ARFLAGS) $@ $^

$(LIBCURVE_OBJS): $(OUTBUILD)/%.o : $(LIB_DIR)/%.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDEDIR)  -c $< -o $@

$(LIBMCLCORE): $(LIBCORE_OBJS)
	$(Q)$(AR) $(ARFLAGS) $@ $^

$(LIBCORE_OBJS): $(OUTBUILD)/%.o : $(LIB_DIR)/%.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDEDIR)  -c $< -o $@

$(STEST_EXE): $(STEST_OBJS) $(LINK_OBJS) $(LIBARACRYPT)
ifeq ($(CONFIG_ARM),y)
	$(Q)$(LD)  -o $@ $@.o $(LINK_OBJS) $(LDFLAGS) -Xlinker -Map=$@.map
else
	$(Q)$(LD)  -o $@ $@.o $(LINK_OBJS) $(LDFLAGS) 
endif
	$(Q)$(SIZE) $@

$(BENCH_EXE): $(BENCH_OBJS) $(LINK_OBJS) $(LIBARACRYPT)
ifeq ($(CONFIG_ARM),y)
	$(Q)$(LD)  -o $@ $@.o $(LINK_OBJS) $(LDFLAGS) -Xlinker -Map=$@.map
else
	$(Q)$(LD)  -o $@ $@.o $(LINK_OBJS) $(LDFLAGS) 
endif
	$(Q)$(SIZE) $@

$(TEST_EXE): $(TEST_OBJS) $(LINK_OBJS) $(LIBARACRYPT)
	$(Q)$(LD)  -o $@ $@.o $(LINK_OBJS) $(LDFLAGS) 
	$(Q)$(SIZE) $@

$(STEST_OBJS): $(OUTBUILD)/%.o : $(TEST_DIR)/%.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDEDIR)  -c $< -o $@

$(TEST_OBJS):  $(OUTBUILD)/%.o : $(TEST_DIR)/%.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDEDIR)  -c $< -o $@

$(BENCH_OBJS):  $(OUTBUILD)/%.o : $(BENCH_DIR)/%.c
	$(Q)$(CC) $(CFLAGS) -D ITERATIONS=$(ITERATIONS) $(INCLUDEDIR)  -c $< -o $@

$(MCL_RSA_OBJS): $(LIB_DIR)/mcl_rsa.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDEDIR)  -c $^ -o $@

$(MCL_ECDH_OBJS): $(LIB_DIR)/mcl_ecdh.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDEDIR)  -c $^ -o $@

$(MCL_UTILS_OBJS): $(TEST_DIR)/mcl_utils.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDEDIR)  -c $^ -o $@

$(MW302_OBJS): $(AWS_SDK)wmsdk/src/boards/mw302_rd.c
	$(Q)$(CC)  $(CFLAGS) $(INCLUDEDIR)  -c $^ -o $@

clean:
	$(Q)-rm -rf $(OUTBUILD)

distclean:
	$(Q)-rm -rf $(OUTBIN)
	$(Q)-rm -rf $(OUTBUILD)

doc:
	$(Q)cd ./docs; doxygen
