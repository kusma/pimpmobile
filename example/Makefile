# Makefile for pimpmobile module player example

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

GBAEMU    ?= vba
DEVKITARM  = $(DEVKITPRO)/devkitARM
LIBGBA     = $(DEVKITPRO)/libgba
PIMPMOBILE = ..

PREFIX ?= arm-elf-
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
LDFLAGS  = -mthumb-interwork -Wl,--gc-section -Wl,-Map,$(basename $@).map -L$(LIBGBA)/lib -lgba libaas.a

ASFLAGS  = -mthumb-interwork

ARM   = -marm
THUMB = -mthumb

OBJS = \
	example.o \
	libgbfs.o \
	AAS_Data.o

MODULES = \
	loop1.xm

#	melkedemo2.mod \
#	swallow-sanovat_etta.xm \
#	eye.mod \
#	rhino.mod \
#	porta2.xm

BIN_MODULES = $(MODULES:=.bin)

TARGET = ../bin/example.gba
CONVERTER = ../bin/converter

ifeq ($(DEBUG), 1)
	CPPFLAGS += -DDEBUG
	CXXFLAGS += -g3 -ggdb
	CFLAGS   += -g3 -ggdb
else
	CPPFLAGS += -DRELEASE -DNDEBUG
	CXXFLAGS += -O3 -fomit-frame-pointer
	CFLAGS   += -O3 -fomit-frame-pointer
endif
	
.PHONY: all clean run debug

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(TARGET:.gba=.elf) $(OBJS) $(OBJS:.o=.d) data.gbfs sample_bank.bin $(BIN_MODULES) *~

run: $(TARGET)
	$(GBAEMU) $(TARGET)

debug: $(TARGET:.gba=.elf)
	$(DEVKITPRO)/insight/bin/arm-elf-insight.exe &
	$(DEVKITPRO)/vba/VisualBoyAdvance-SDL.exe -Gtcp:55555 example.elf

data.gbfs: sample_bank.bin $(BIN_MODULES)

$(CONVERTER)::
	make -C ../converter

$(PIMPMOBILE)/lib/libpimpmobile.a::
	make -C .. lib/libpimpmobile.a

$(TARGET:.gba=.elf): $(OBJS) $(PIMPMOBILE)/lib/libpimpmobile.a

$(BIN_MODULES) sample_bank.bin : $(MODULES) $(CONVERTER)
	$(CONVERTER) $(MODULES)

%.gbfs:
	gbfs $@ $^

%.elf:
	$(LD) -specs=gba.specs $^ $(LDFLAGS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	padbin 256 $@
	gbafix $@ -t$(notdir $(basename $@))

%.gba: %.bin data.gbfs
	cat $^ > $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THUMB) -c $< -o $@ -MMD -MF $(@:.o=.d)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THUMB) -c $< -o $@ -MMD -MF $(@:.o=.d)

-include $(OBJS:.o=.d)
