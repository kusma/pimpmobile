#include "framework/test.h"
#include <stdio.h>

void test_mixer(void);
void test_serializer(void);

int main(int argc, char *argv[])
{
	test_mixer();
	test_serializer();
	return test_report_file(stdout);
}
