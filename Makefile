# Makefile for pimpmobile module player

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

GBAEMU   ?= vba
DEVKITARM = $(DEVKITPRO)/devkitARM
LIBGBA    = $(DEVKITPRO)/libgba

PREFIX ?= arm-elf-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
STRIP   = $(PREFIX)strip
LD      = $(PREFIX)g++
AS      = $(PREFIX)gcc
AR      = $(PREFIX)ar

CPPFLAGS = -I$(DEVKITARM)/include -I$(LIBGBA)/include
CFLAGS   = -mthumb-interwork -mlong-calls
CXXFLAGS = -mthumb-interwork -mlong-calls -fconserve-space -fno-rtti -fno-exceptions
LDFLAGS  = -mthumb-interwork -Wl,--gc-section -Wl,-Map,$(basename $@).map
ASFLAGS  = -mthumb-interwork

ARM   = -marm
THUMB = -mthumb

ifeq ($(DEBUG), 1)
	CPPFLAGS += -DDEBUG
else
	CPPFLAGS += -DRELEASE -DNDEBUG
	CXXFLAGS += -O3 -fomit-frame-pointer
	CFLAGS   += -O3 -fomit-frame-pointer
endif

LIBS = $(LIBGBA)/lib/libgba.a
OBJS = \
	src/pimpmobile.o \
	src/mixer.iwram.o

.PHONY: all clean

all: example.gba

clean:
	$(RM) example.elf example.gba example.o example.map sample.o converter converter.exe $(OBJS) lib/libpimpmobile.a *~ src/*~ include/*~

run: example.gba
	$(GBAEMU) example.gba

converter: converter.cpp
	g++ converter.cpp -o converter

example.gba: converter
example.elf: example.o sample.o lib/libpimpmobile.a
lib/libpimpmobile.a: $(OBJS)

%.a:
	$(AR) $(ARFLAGS) $@ $?

%.elf:
	$(LD) $(LDFLAGS) -specs=gba.specs $^ $(LIBS) -o $@

%.gba: %.elf
	$(OBJCOPY) -O binary $< $@
	gbafix $@ -t$(basename $@)

%.iwram.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(ARM) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THUMB) -c $< -o $@

%.iwram.s: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -S -fverbose-asm $(ARM) -c $< -o $@

%.s: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -S -fverbose-asm $(THUMB) -c $< -o $@

%.o: %.raw
	arm-elf-objcopy -I binary -O elf32-littlearm \
	--rename-section .data=.rodata,readonly,data,contents,alloc \
	--redefine-sym _binary_$*_raw_start=$* \
	--redefine-sym _binary_$*_raw_end=$*_end \
	--redefine-sym _binary_$*_raw_size=$*_size \
	-B arm $< $@
