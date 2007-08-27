/* pimp_math.c -- Math routines for use in Pimpmobile
 * Copyright (C) 2007 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include "framework/test_framework.h"
#include <stdio.h>

void test_mixer(struct test_suite *suite);
void test_serializer(struct test_suite *suite);

int main(int argc, char *argv[])
{
	struct test_suite suite;
	
	asdtest_mixer(&suite);
	test_serializer(&suite);
	
	return test_report_file(&suite, stderr);
}
