# Makefile for pimpmobile module player file converter

CC      = gcc
CXX     = g++
STRIP   = strip
LD      = g++
AS      = as

CPPFLAGS = 
CFLAGS   = 
CXXFLAGS = 
LDFLAGS  = 
ASFLAGS  = 

OBJS = \
	converter.o \
	converter_xm.o \
	converter_s3m.o \
	converter_mod.o \
	dump_module.o \
	dump_samples.o

TARGET = ../bin/converter

ifeq ($(DEBUG), 1)
	CPPFLAGS += -DDEBUG
	CXXFLAGS += -g3 -ggdb
	CFLAGS   += -g3 -ggdb
else
	CPPFLAGS += -DRELEASE -DNDEBUG
	CXXFLAGS += -O3 -fomit-frame-pointer
	CFLAGS   += -O3 -fomit-frame-pointer
endif
	
.PHONY: all clean

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJS) $(OBJS:.o=.d) *~

$(TARGET): $(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d)

-include $(OBJS:.o=.d)
