# Makefile for pimpmobile module player
# Copyright (C) 2005-2007 Jørn Nystad and Erik Faye-Lund
# For conditions of distribution and use, see copyright notice in LICENSE.TXT

CC = gcc
CPPFLAGS = -I.. -DASSERT_ENABLE -DUNIT_TESTING -DDEBUG -g

FRAMEWORK_SOURCES = \
	framework/test_framework.c \
	framework/test_helpers.c

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
	../converter/load_xm.c \
	../converter/convert_sample.c \
	../src/pimp_sample_bank.c

.PHONY: clean all
all: unit_test

clean:
	$(RM) unit_test

run: unit_test
	./unit_test

unit_test: $(TEST_SOURCES) $(TEST_DEPS_SOURCES) $(FRAMEWORK_SOURCES)

toplevel/dump_render: $(TEST_DEPS_SOURCES)
