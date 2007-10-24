# Makefile for pimpmobile module player
# Copyright (C) 2005-2007 Jørn Nystad and Erik Faye-Lund
# For conditions of distribution and use, see copyright notice in LICENSE.TXT

# default-configuration
TARGET    ?= arm-gba
HOST      ?= $(shell $(CC) -dumpmachine)
BUILD_DIR ?= build
CONFIG    ?= release

# some common unix utils
AWK   ?= awk
SORT  ?= sort
PR    ?= pr
CTAGS ?= ctags

#ifeq ($(strip $(DEVKITPRO)),)
#$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
#endif

ifneq ($(strip $(DEVKITPRO)),)
DEVKITARM = $(DEVKITPRO)/devkitARM
LIBGBA    = $(DEVKITPRO)/libgba
LIBNDS    = $(DEVKITPRO)/libnds
export PATH	:=	$(DEVKITARM)/bin:$(PATH)
endif

ifdef COMSPEC
EXE_EXT=.exe
else
EXE_EXT=
endif

ifeq ($(TARGET), arm-gba)
TARGET_PREFIX ?= arm-eabi-
else
TARGET_PREFIX = $(TARGET)
endif

#$(eval $(call setup-gcc, out-prefix, toolchain-prefix))
define setup-gcc
	$(1)CC      = $(2)gcc
	$(1)CXX     = $(2)g++
	$(1)STRIP   = $(2)strip
	$(1)LD      = $(2)ld
	$(1)AS      = $(2)as
	$(1)AR      = $(2)ar
endef

#$(eval $(call setup-armcc, out-prefix))
define setup-armcc
	$(1)CC      = armcc
	$(1)CXX     = armcpp
	$(1)STRIP   = strip
	$(1)LD      = armlink
	$(1)AS      = armasm
	$(1)AR      = armar
endef

$(eval $(call setup-gcc,TARGET_, $(TARGET_PREFIX)))
# $(eval $(call setup-armcc,TARGET_))
$(eval $(call setup-gcc,,))

MKDIR = mkdir -p

ifeq ($(TARGET), arm-gba)
	TARGET_CPPFLAGS = -I$(DEVKITARM)/include -I$(LIBGBA)/include -DTARGET_GBA
	TARGET_CFLAGS   = -mthumb-interwork -mlong-calls
	TARGET_CXXFLAGS = -mthumb-interwork -mlong-calls
	TARGET_LDFLAGS  = -mthumb-interwork -Wl,--gc-section
	TARGET_ASFLAGS  = -mthumb-interwork
endif

CPPFLAGS = 
CFLAGS   = -pedantic -Wall -Wno-long-long
CXXFLAGS = -fconserve-space -fno-rtti -fno-exceptions
LDFLAGS  =
ASFLAGS  = 

ifeq ($(HOST), x86_64-linux-gnu)
	HOST_CFLAGS += -m32
	HOST_CXXFLAGS += -m32
	HOST_LDFLAGS += -m32
endif

ARM   = -marm
THUMB = -mthumb

SOURCES = \
	src/pimp_render.c      \
	src/pimp_envelope.c    \
	src/pimp_mod_context.c \
	src/pimp_math.c        \
	src/pimp_mixer.c

ifeq ($(TARGET), arm-gba)
	SOURCES += src/pimp_gba.c
endif

ARM_SOURCES = \
	src/pimp_math.c \
	src/pimp_mixer.c

PIMPCONV_SOURCES = \
	converter/pimpconv.c \
	converter/serializer.cpp \
	converter/serialize_module.c \
	converter/serialize_instrument.c \
	converter/convert_sample.c \
	converter/load_xm.c \
	converter/load_mod.c \
	src/pimp_sample_bank.c

ifeq ($(CONFIG), debug)
	CPPFLAGS += -DDEBUG
	CXXFLAGS += -g -ggdb
	CFLAGS   += -g -ggdb
	SOURCES  += src/pimp_mixer_portable.c
	SOURCES  += src/pimp_debug.c
else
	CPPFLAGS += -DRELEASE -DNDEBUG
	CXXFLAGS += -O3 -fomit-frame-pointer
	CFLAGS   += -O3 -fomit-frame-pointer
	SOURCES  += src/pimp_mixer_arm.S
	SOURCES  += src/pimp_mixer_clip_arm.S
endif

ifeq ($(PROFILING), 1)
	CFLAGS   += -finstrument-functions
	CXXFLAGS += -finstrument-functions
	SOURCES  += profiling/cyg-profile.c
endif

TARGET_BUILD_DIR = $(BUILD_DIR)/$(TARGET)/$(CONFIG)
HOST_BUILD_DIR = $(BUILD_DIR)/$(HOST)/$(CONFIG)

source-to-object = \
	$(subst .c,.o,        $(filter-out $(ARM_SOURCES), $(filter %.c,$1))) \
	$(subst .c,.iwram.o,  $(filter     $(ARM_SOURCES), $(filter %.c,$1))) \
	$(subst .cpp,.o,      $(filter-out $(ARM_SOURCES), $(filter %.cpp,$1))) \
	$(subst .cpp,.iwram.o,$(filter     $(ARM_SOURCES), $(filter %.cpp,$1))) \
	$(subst .S,.o,        $(filter-out $(ARM_SOURCES), $(filter %.S,$1))) \
	$(subst .S,.iwram.o,  $(filter     $(ARM_SOURCES), $(filter %.S,$1)))

source-to-depend = $(subst .o,.d, $(call source-to-object, $1))

make-target-objs = $(addprefix $(TARGET_BUILD_DIR)/, $(call source-to-object, $1))
make-host-objs   = $(addprefix $(HOST_BUILD_DIR)/,   $(call source-to-object, $1))

make-target-deps = $(addprefix $(TARGET_BUILD_DIR)/, $(call source-to-depend, $1))
make-host-deps   = $(addprefix $(HOST_BUILD_DIR)/,   $(call source-to-depend, $1))

OBJS = $(call make-target-objs, $(SOURCES))

.PHONY: all clean help check check-syntax

all: lib/libpimpmobile.a

clean:
	$(RM) lib/libpimpmobile.a $(call make-target-objs, $(SOURCES)) $(call make-target-deps, $(SOURCES))
	$(RM) bin/pimpconv$(EXE_EXT) $(call make-host-objs, $(PIMPCONV_SOURCES)) $(call make-host-deps, $(PIMPCONV_SOURCES))

distclean:
	$(RM) -r $(BUILD_DIR)

help:
	@echo available targets:
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) --print-data-base --question | \
	$(AWK) '/^[^.%!][a-zA-Z0-9/_-]*:/                                       \
	       { print substr($$1, 1, length($$1)-1) }' |                       \
	$(SORT) |                                                               \
	$(PR) --omit-pagination --width=80 --columns=4

check:
	@echo no testbench atm

check-syntax:
	$(TARGET_CC) $(CPPFLAGS) $(TARGET_CPPFLAGS) $(CFLAGS) -fsyntax-only $(filter %.c,$(SOURCES))

TAGS:
	$(CTAGS) $(filter %.c,$(SOURCES))

$(call make-target-objs, $(filter-out $(ARM_SOURCES), $(SOURCES))): TARGET_CFLAGS += -mthumb
$(call make-target-objs, $(filter     $(ARM_SOURCES), $(SOURCES))): TARGET_CFLAGS += -marm

bin/pimpconv$(EXE_EXT): CC = g++ # make sure we use the c++ compiler for this
bin/pimpconv$(EXE_EXT): $(call make-host-objs, $(PIMPCONV_SOURCES))
	$(LINK.o) $^ $(LOADLIBES) $(OUTPUT_OPTION)

lib/libpimpmobile.a: $(OBJS)
	$(TARGET_AR) $(ARFLAGS) $@ $?

# set compiler for ARM-sources
# $(call make-target-objs, $(filter-out $(ARM_SOURCES), $(SOURCES))):	CC = $(TARGET_CC)
# $(call make-target-objs, $(filter-out $(ARM_SOURCES), $(SOURCES))): CFLAGS = $(TARGET_CFLAGS)
#$(call make-target-objs, $(filter-out $(ARM_SOURCES), $(SOURCES))):
#	echo $(COMPILE.c) $<

# Override CC for target-builds
$(TARGET_BUILD_DIR)/%.o: CC = $(TARGET_CC)
$(TARGET_BUILD_DIR)/%.o: CPPFLAGS += $(TARGET_CPPFLAGS)
$(TARGET_BUILD_DIR)/%.o: CFLAGS += $(TARGET_CFLAGS)

$(HOST_BUILD_DIR)/%.o: CFLAGS += $(HOST_CFLAGS)
$(HOST_BUILD_DIR)/%.o: CXXFLAGS += $(HOST_CXXFLAGS)
bin/pimpconv$(EXE_EXT): CC = $(CXX)
bin/pimpconv$(EXE_EXT): LDFLAGS += $(HOST_LDFLAGS) -lstdc++

#### C ####

$(TARGET_BUILD_DIR)/%.iwram.o: %.c
	@$(MKDIR) $(dir $@)
	$(COMPILE.c) $(OUTPUT_OPTION) $< -MMD -MP -MF $(@:.o=.d)

$(TARGET_BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(COMPILE.c) $(OUTPUT_OPTION) $< -MMD -MP -MF $(@:.o=.d)

$(HOST_BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(COMPILE.c) $(OUTPUT_OPTION) $< -MMD -MP -MF $(@:.o=.d)

### C++

$(TARGET_BUILD_DIR)/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(TARGET_CXX) $(CPPFLAGS) $(CXXFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

$(HOST_BUILD_DIR)/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

### ASM

$(TARGET_BUILD_DIR)/%.o: %.S
	@$(MKDIR) $(dir $@)
	$(TARGET_CC) -MMD -MF $(@:.o=.d) -x assembler-with-cpp -trigraphs $(ASFLAGS) -c $< -o $@

# deps
-include $(call make-target-deps, $(SOURCES))
-include $(call make-host-deps, $(PIMPCONV_SOURCES))
