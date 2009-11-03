#include "test_helpers.h"
#include "test_framework.h"

#include <stdio.h>
#include <stdlib.h>

void test_int_array(struct test_suite *suite, const int *array, const int *reference, int size, const char *file, int line)
{
	char temp[1024];
	int err = 0;
	
	int i;
	for (i = 0; i < size; ++i)
	{
		char val = array[i];
		char ref = reference[i];
		if (val != ref)
		{
			snprintf(temp, 1024, "%s:%d -- element #%d not equal, got %X - expected %X", file, line, i + 1, val, ref);
			err = 1;
			break;
		}
	}
	
	if (0 != err) test_fail(suite, temp);
	else test_pass(suite);
}
