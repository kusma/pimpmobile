/* unit_test.c -- Main entry for Pimpmobile unit tests
 * Copyright (C) 2007 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include "framework/test_framework.h"
#include "framework/test_helpers.h"
#include <stdio.h>

void test_mixer(struct test_suite *suite);
void test_serializer(struct test_suite *suite);

int main(int argc, char *argv[])
{
	/* setup test suite */
	struct test_suite suite;
	test_suite_init(&suite);
	
	/* run tests */
	TEST_RUN(&suite, test_mixer);
	TEST_RUN(&suite, test_serializer);
	
	/* report */
	return test_report_file(&suite, stderr);
}
