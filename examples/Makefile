# Makefile for pimpmobile module player examples
# Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
# For conditions of distribution and use, see copyright notice in LICENSE.TXT

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

ifdef COMSPEC
EXE_EXT=.exe
else
EXE_EXT=
endif

DEVKITARM  = $(DEVKITPRO)/devkitARM
LIBGBA     = $(DEVKITPRO)/libgba
export PATH := $(DEVKITARM)/bin:$(PATH)

PIMPMOBILE = ..

PREFIX ?= arm-none-eabi-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
STRIP   = $(PREFIX)strip
LD      = $(PREFIX)g++
AS      = $(PREFIX)as
AR      = $(PREFIX)ar

CPPFLAGS = -I$(DEVKITARM)/include -I$(LIBGBA)/include -I$(PIMPMOBILE)/include
TARGET_COMMON = -mcpu=arm7tdmi -mtune=arm7tdmi -mthumb-interwork
CFLAGS   = $(TARGET_COMMON) -mlong-calls
CXXFLAGS = $(TARGET_COMMON) -mlong-calls -fconserve-space -fno-rtti -fno-exceptions
LDFLAGS  = $(TARGET_COMMON) -Wl,--gc-section
ASFLAGS  = $(TARGET_COMMON)
LDLIBS   = -L$(PIMPMOBILE)/lib -lpimp_gba -L$(LIBGBA)/lib -lgba
TARGET_ARCH = -specs=gba.specs
CONVERTER = $(PIMPMOBILE)/bin/pimpconv$(EXE_EXT)

ARM   = -marm
THUMB = -mthumb

EXAMPLES = example example2

.PHONY: all clean run
all: $(EXAMPLES:=.gba)

example-data.gbfs: data/dxn-oopk.xm
	$(CONVERTER) $<
	gbfs $@ sample_bank.bin $(<:=.bin)

example2-data.gbfs: data/dxn-oopk.xm

EXTRA_OBJS = \
	libgbfs.o \
	gbfs_stdio.o \
	../src/load_xm.o \
	../src/convert_sample.o \
	../src/pimp_sample_bank.o

ifeq ($(DEBUG), 1)
	CPPFLAGS += -DDEBUG
	CXXFLAGS += -g3 -ggdb
	CFLAGS   += -g3 -ggdb
else
	CPPFLAGS += -DRELEASE -DNDEBUG
	CXXFLAGS += -O3 -fomit-frame-pointer
	CFLAGS   += -O3 -fomit-frame-pointer
endif

clean:
	$(RM) $(EXAMPLES:=.gba) $(EXAMPLES:=-data.gbfs) $(EXTRA_OBJS) \
	$(EXTRA_OBJS:.o=.d) sample_bank.bin

%.gbfs:
	gbfs $@ $<

%.elf: %.cpp $(EXTRA_OBJS)
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	padbin 256 $@
	gbafix $@ -t$(basename $@)

%.gba: %.bin %-data.gbfs
	cat $^ > $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

-include $(OBJS:.o=.d)
