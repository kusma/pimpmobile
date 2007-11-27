#include "test_suite.h"

#include <stddef.h>
#include <assert.h>
#define ASSERT assert

void test_suite_init(struct test_suite *test_suite)
{
	ASSERT(NULL != test_suite);
	
	test_suite->test_count = 0;
	test_suite->fail_count = 0;
	test_suite->pass_count = 0;
}
