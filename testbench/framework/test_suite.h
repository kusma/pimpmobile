#ifndef TEST_SUITE_H
#define TEST_SUITE_H

struct test_suite
{
	int test_count;
	int fail_count;
	int pass_count;
};

void test_suite_init(struct test_suite *test_suite);

#endif /* TEST_SUITE_H */
