CPPFLAGS = -I.. -DASSERT_ENABLE -DUNIT_TESTING -DDEBUG -g

FRAMEWORK_SOURCES = \
	framework/test.c \
	framework/test_helpers.c

TEST_SOURCES = \
	unit_test.c \
	unit/test_mixer.c \
	unit/test_serializer.c

TEST_DEPS_SOURCES = \
	../src/pimp_mixer.c \
	../src/pimp_mixer_portable.c \
	../converter/serializer.cpp

.PHONY: clean all
all: unit_test

clean:
	$(RM) unit_test

run: unit_test
	./unit_test

unit_test: $(TEST_SOURCES) $(TEST_DEPS_SOURCES) $(FRAMEWORK_SOURCES)
