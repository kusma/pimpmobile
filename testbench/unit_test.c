#include "framework/test_framework.h"
#include <stdio.h>

void test_mixer(struct test_suite *suite);
void test_serializer(struct test_suite *suite);

int main(int argc, char *argv[])
{
	struct test_suite suite;
	
	test_mixer(&suite);
	test_serializer(&suite);
	
	return test_report_file(&suite, stderr);
}
