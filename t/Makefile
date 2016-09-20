# Makefile for pimpmobile module player
# Copyright (C) 2005-2007 Jørn Nystad and Erik Faye-Lund
# For conditions of distribution and use, see copyright notice in LICENSE.TXT

CC = g++
CPPFLAGS = -I.. -DUNIT_TESTING -DDEBUG
CFLAGS   = -g
CXXFLAGS = -g

ifdef COMSPEC
EXE_EXT=.exe
else
EXE_EXT=
endif

FRAMEWORK_SOURCES = \
	framework/test_framework.c \
	framework/test_helpers.c \
	framework/test_suite.c

TEST_SOURCES = \
	unit_test.c \
	unit/test_mixer.c \
	unit/test_serializer.c

TEST_DEPS_SOURCES = \
	../src/pimp_mixer.c \
	../src/pimp_mod_context.c \
	../src/pimp_mixer_portable.c \
	../src/pimp_envelope.c \
	../src/pimp_math.c \
	../src/pimp_render.c \
	../src/load_xm.c \
	../src/convert_sample.c \
	../src/pimp_sample_bank.c

.PHONY: clean all run
all: unit_test$(EXE_EXT) dump_render$(EXE_EXT)

clean:
	$(RM) unit_test$(EXE_EXT) dump_render$(EXE_EXT)

run: unit_test$(EXE_EXT)
	./unit_test$(EXE_EXT)

unit_test$(EXE_EXT): $(TEST_SOURCES) $(TEST_DEPS_SOURCES) $(FRAMEWORK_SOURCES) ../converter/serializer.cpp
	$(LINK.cpp) $^$(LOADLIBES) $(LDLIBS) -o $@

dump_render$(EXE_EXT): toplevel/dump_render.c $(TEST_DEPS_SOURCES)
	$(LINK.c) $^$(LOADLIBES) $(LDLIBS) -o $@

%.wav : %.sb
	sox -r 44100 $< $@
