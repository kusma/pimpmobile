# Makefile for pimpmobile module player example
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

PREFIX ?= arm-eabi-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
STRIP   = $(PREFIX)strip
LD      = $(PREFIX)g++
AS      = $(PREFIX)as
AR      = $(PREFIX)ar

CPPFLAGS = -I$(DEVKITARM)/include -I$(LIBGBA)/include -I$(PIMPMOBILE)/include
CFLAGS   = -mthumb-interwork -mlong-calls
CXXFLAGS = -mthumb-interwork -mlong-calls -fconserve-space -fno-rtti -fno-exceptions
LDFLAGS  = -mthumb-interwork -Wl,--gc-section -Wl,-Map,$(basename $@).map -L$(LIBGBA)/lib -lgba
ASFLAGS  = -mthumb-interwork

ARM   = -marm
THUMB = -mthumb

OBJS = \
	example.o \
	libgbfs.o \
	gbfs_stdio.o

MODULES = $(shell find data -iname "*.xm" -or -iname "*.mod")

BIN_MODULES = $(MODULES:=.bin)

TARGET = ../bin/example.gba
CONVERTER = ../bin/pimpconv$(EXE_EXT)

ifeq ($(DEBUG), 1)
	CPPFLAGS += -DDEBUG
	CXXFLAGS += -g3 -ggdb
	CFLAGS   += -g3 -ggdb
else
	CPPFLAGS += -DRELEASE -DNDEBUG
	CXXFLAGS += -O3 -fomit-frame-pointer
	CFLAGS   += -O3 -fomit-frame-pointer
endif
	
.PHONY: all clean run

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(TARGET:.gba=.elf) $(OBJS) $(OBJS:.o=.d) data.gbfs sample_bank.bin $(BIN_MODULES) *~

data.gbfs: sample_bank.bin $(BIN_MODULES)

data2.gbfs: $(MODULES)

$(CONVERTER)::
	make -C .. bin/pimpconv$(EXE_EXT)

$(TARGET:.gba=.elf): $(OBJS) $(PIMPMOBILE)/lib/libpimp_gba.a

$(BIN_MODULES) sample_bank.bin : $(MODULES) $(CONVERTER)
	$(CONVERTER) $(MODULES)

%.gbfs:
	gbfs $@ $^

%.elf:
	$(LD) -specs=gba.specs $^ $(LDFLAGS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	padbin 256 $@
	gbafix $@ -t$(basename $@)

%.gba: %.bin data.gbfs
	cat $^ > $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

-include $(OBJS:.o=.d)
