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

ifneq ($(strip $(DEVKITPRO)),)
DEVKITARM = $(DEVKITPRO)/devkitARM
export PATH := $(DEVKITARM)/bin:$(PATH)
endif

ifdef COMSPEC
EXE_EXT = .exe
else
EXE_EXT =
endif

ifeq ($(TARGET), arm-gba)
TARGET_PREFIX ?= arm-none-eabi-
else
TARGET_PREFIX = $(TARGET)
endif

define setup-gcc
	$(1)CC    = $(2)gcc
	$(1)STRIP = $(2)strip
	$(1)LD    = $(2)ld
	$(1)AS    = $(2)as
	$(1)AR    = $(2)ar
endef

define setup-armcc
	$(1)CC    = armcc
	$(1)STRIP = strip
	$(1)LD    = armlink
	$(1)AS    = armasm
	$(1)AR    = armar
endef

ifdef USE_ARMCC
$(eval $(call setup-armcc,TARGET_))
else
$(eval $(call setup-gcc,TARGET_, $(TARGET_PREFIX)))
endif

$(eval $(call setup-gcc,,))

ifneq ($(findstring $(MAKEFLAGS),s),s)
	QUIET_CC   = @echo '   ' CC $@;
	QUIET_AS   = @echo '   ' AS $@;
	QUIET_AR   = @echo '   ' AR $@;
	QUIET_LINK = @echo '   ' LINK $@;
endif

MKDIR = mkdir -p

ifeq ($(TARGET), arm-gba)
	TARGET_CPPFLAGS = -I$(DEVKITARM)/include -DTARGET_GBA
	TARGET_COMMON   = -mcpu=arm7tdmi -mtune=arm7tdmi -mthumb-interwork
	TARGET_CFLAGS   = $(TARGET_COMMON) -mlong-calls
	TARGET_LDFLAGS  = $(TARGET_COMMON) -Wl,--gc-section
	TARGET_ASFLAGS  = $(TARGET_COMMON)
endif

CPPFLAGS =
CFLAGS   = -pedantic -Wall -Wno-long-long
LDFLAGS  =
ASFLAGS  =
ARFLAGS  = rcs

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
	converter/serializer.c \
	converter/serialize_module.c \
	converter/serialize_instrument.c \
	src/convert_sample.c \
	src/load_xm.c \
	src/load_mod.c \
	src/pimp_sample_bank.c

ifeq ($(CONFIG), debug)
	CPPFLAGS += -DDEBUG
	CFLAGS   += -g -ggdb
	SOURCES  += src/pimp_mixer_portable.c
	SOURCES  += src/pimp_debug.c
else
	CPPFLAGS += -DRELEASE -DNDEBUG
	CFLAGS   += -O3 -fomit-frame-pointer
	SOURCES  += src/pimp_mixer_arm.S
	SOURCES  += src/pimp_mixer_clip_arm.S
endif

ifeq ($(PROFILING), 1)
	CFLAGS   += -finstrument-functions
	SOURCES  += profiling/cyg-profile.c
endif

TARGET_BUILD_DIR = $(BUILD_DIR)/$(TARGET)/$(CONFIG)
HOST_BUILD_DIR = $(BUILD_DIR)/$(HOST)/$(CONFIG)

source-to-object = \
	$(subst .c,.o,        $(filter-out $(ARM_SOURCES), $(filter %.c,$1))) \
	$(subst .c,.iwram.o,  $(filter     $(ARM_SOURCES), $(filter %.c,$1))) \
	$(subst .S,.o,        $(filter-out $(ARM_SOURCES), $(filter %.S,$1))) \
	$(subst .S,.iwram.o,  $(filter     $(ARM_SOURCES), $(filter %.S,$1)))

source-to-depend = $(subst .o,.d, $(call source-to-object, $1))

make-target-objs = $(addprefix $(TARGET_BUILD_DIR)/, $(call source-to-object, $1))
make-host-objs   = $(addprefix $(HOST_BUILD_DIR)/,   $(call source-to-object, $1))

make-target-deps = $(addprefix $(TARGET_BUILD_DIR)/, $(call source-to-depend, $1))
make-host-deps   = $(addprefix $(HOST_BUILD_DIR)/,   $(call source-to-depend, $1))

OBJS = $(call make-target-objs, $(SOURCES))

.PHONY: all clean check check-syntax examples

all: lib/libpimp_gba.a bin/pimpconv$(EXE_EXT)

clean:
	$(RM) lib/libpimp_gba.a $(call make-target-objs, $(SOURCES)) $(call make-target-deps, $(SOURCES))
	$(RM) bin/pimpconv$(EXE_EXT) $(call make-host-objs, $(PIMPCONV_SOURCES)) $(call make-host-deps, $(PIMPCONV_SOURCES))
	$(MAKE) -C examples clean

distclean:
	$(RM) -r $(BUILD_DIR)

check:
	$(MAKE) -C t run

check-syntax:
	$(TARGET_CC) $(CPPFLAGS) $(TARGET_CPPFLAGS) $(CFLAGS) -fsyntax-only $(filter %.c,$(SOURCES))

examples: lib/libpimp_gba.a bin/pimpconv$(EXE_EXT)
	$(MAKE) -C examples

TAGS:
	$(CTAGS) $(filter %.c,$(SOURCES))

$(call make-target-objs, $(filter-out $(ARM_SOURCES), $(SOURCES))): TARGET_CFLAGS += -mthumb
$(call make-target-objs, $(filter     $(ARM_SOURCES), $(SOURCES))): TARGET_CFLAGS += -marm

bin/pimpconv$(EXE_EXT): $(call make-host-objs, $(PIMPCONV_SOURCES))
	$(QUIET_LINK)$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

lib/libpimp_gba.a: $(OBJS)
	$(QUIET_AR)$(TARGET_AR) $(ARFLAGS) $@ $?

# Override CC for target-builds
$(TARGET_BUILD_DIR)/%.o: CC = $(TARGET_CC)
$(TARGET_BUILD_DIR)/%.o: CPPFLAGS += $(TARGET_CPPFLAGS)
$(TARGET_BUILD_DIR)/%.o: CFLAGS += $(TARGET_CFLAGS)
$(TARGET_BUILD_DIR)/%.o: ASFLAGS += $(TARGET_ASFLAGS)

# Override CC for host-builds
bin/pimpconv$(EXE_EXT): LOADLIBES += -lm

### C

$(TARGET_BUILD_DIR)/%.iwram.o: %.c
	@$(MKDIR) $(dir $@)
	$(QUIET_CC)$(COMPILE.c) $(OUTPUT_OPTION) $< -MMD -MP -MF $(@:.o=.d)

$(TARGET_BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(QUIET_CC)$(COMPILE.c) $(OUTPUT_OPTION) $< -MMD -MP -MF $(@:.o=.d)

$(HOST_BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(QUIET_CC)$(COMPILE.c) $(OUTPUT_OPTION) $< -MMD -MP -MF $(@:.o=.d)

### ASM

$(TARGET_BUILD_DIR)/%.o: %.S
	@$(MKDIR) $(dir $@)
	$(QUIET_AS)$(COMPILE.S) $(OUTPUT_OPTION) $< -MMD -MP -MF $(@:.o=.d)

# deps
-include $(call make-target-deps, $(SOURCES))
-include $(call make-host-deps, $(PIMPCONV_SOURCES))
