#include "test_framework.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TEST_PRINTF_STRING_LEN 4096

char *test_printf(struct test_suite *suite, const char* fmt, ...)
{
	char temp[MAX_TEST_PRINTF_STRING_LEN];
	char *string;
	int len = 0;
	va_list arglist;
	
	va_start(arglist, fmt);
	len = vsnprintf(temp, MAX_TEST_PRINTF_STRING_LEN, fmt, arglist);
	va_end(arglist);

	string = (char*)malloc(len + 1);
	if (NULL != string)
	{
		va_start(arglist, fmt);
		vsnprintf(string, len + 1, fmt, arglist);
		va_end(arglist);
		string[len] = 0; /* ensure string termination */
	}
	
	return string;
}

int test_test_count = 0;
int test_fail_count = 0;
int test_pass_count = 0;

int test_fail(struct test_suite *suite, const char *error)
{
	printf("TEST #%d FAILED: %s\n", test_test_count + 1, error);
	test_test_count++;
	test_fail_count++;
	return test_test_count;
}

int test_pass(struct test_suite *suite)
{
	test_test_count++;
	test_pass_count++;
	return test_test_count;
}

int test_report_file(struct test_suite *suite, FILE *fp)
{
	fprintf(fp, "%d/%d tests failed\n", test_fail_count, test_test_count);
	return test_fail_count;
}