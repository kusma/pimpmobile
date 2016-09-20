#include "test_framework.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define ASSERT assert

#define MAX_TEST_PRINTF_STRING_LEN 4096

void  test_run(struct test_suite *suite, void (*test_func)(struct test_suite *), const char *name)
{
	int failures_before;
	int failures_after;
	
	ASSERT(NULL != suite);
	ASSERT(NULL != test_func);
	
	failures_before = suite->fail_count;
	test_func(suite);
	failures_after  = suite->fail_count;
	
	if (failures_before != failures_after)
	{
		printf("test \"%s\" failed.\n", name);
	}
}

char *test_printf(struct test_suite *suite, const char* fmt, ...)
{
	char temp[MAX_TEST_PRINTF_STRING_LEN];
	char *string;
	int len = 0;
	va_list arglist;
	
	va_start(arglist, fmt);
	len = vsnprintf(temp, MAX_TEST_PRINTF_STRING_LEN, fmt, arglist);
	va_end(arglist);

	string = malloc(len + 1);
	if (NULL != string)
	{
		va_start(arglist, fmt);
		vsnprintf(string, len + 1, fmt, arglist);
		va_end(arglist);
		string[len] = 0; /* ensure string termination */
	}
	
	return string;
}

int test_fail(struct test_suite *suite, const char *error)
{
	ASSERT(NULL != suite);
	
	printf("TEST #%d FAILED: %s\n", suite->test_count + 1, error);
	suite->test_count++;
	suite->fail_count++;
	
	return suite->test_count;
}

int test_pass(struct test_suite *suite)
{
	ASSERT(NULL != suite);
	
	suite->test_count++;
	suite->pass_count++;
	
	return suite->test_count;
}

int test_report_file(struct test_suite *suite, FILE *fp)
{
	ASSERT(NULL != suite);
	ASSERT(NULL != fp);
	
	fprintf(fp, "%d/%d tests failed\n", suite->fail_count, suite->test_count);
	return suite->fail_count;
}
