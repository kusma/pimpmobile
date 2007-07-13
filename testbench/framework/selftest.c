#include "test_framework.h"
#include "test_helpers.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	struct test_suite suite;
	ASSERT_INTS_EQUAL(&suite, 1, 0);
	return test_report_file(&suite, stderr);
}
