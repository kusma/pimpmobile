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
AS      = $(PREFIX)as
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
	CXXFLAGS += -g3 -ggdb
	CFLAGS   += -g3 -ggdb
else
	CPPFLAGS += -DRELEASE -DNDEBUG
	CXXFLAGS += -O3 -fomit-frame-pointer
	CFLAGS   += -O3 -fomit-frame-pointer
endif

LIBS = $(LIBGBA)/lib/libgba.a
OBJS = \
	src/pimpmobile.o \
	src/mixer.iwram.o \
	src/mixer_arm.iwram.o

EXAMPLE_OBJS = \
	example/example.o \
	example/data.o
	
.PHONY: all clean

all: bin/example.gba

clean:
	$(RM) bin/example.elf bin/example.gba $(EXAMPLE_OBJS) $(OBJS) lib/libpimpmobile.a *~ src/*~ include/*~

run: bin/example.gba
	$(GBAEMU) bin/example.gba
	
debug: example.elf
	$(DEVKITPRO)/insight/bin/arm-elf-insight.exe &
	$(DEVKITPRO)/vba/VisualBoyAdvance-SDL.exe -Gtcp:55555 example.elf

bin/converter: converter/converter.cpp converter/converter_xm.cpp converter/converter_s3m.cpp converter/converter_mod.cpp converter/converter.h
	g++ converter/converter.cpp converter/converter_xm.cpp converter/converter_s3m.cpp converter/converter_mod.cpp -o bin/converter

bin/lut_gen: lut_gen.cpp src/config.h
	g++ lut_gen.cpp -o bin/lut_gen

bin/example.gba: converter
bin/example.elf: $(EXAMPLE_OBJS) lib/libpimpmobile.a
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
