#ifndef TEST_H
#define TEST_H

#include "test_suite.h"

int   test_fail(struct test_suite *suite, const char *error);
int   test_pass(struct test_suite *suite);
char *test_printf(struct test_suite *suite, const char* fmt, ...);

#include <stdio.h>
int test_report_file(struct test_suite *suite, FILE *fp);

/* #define ASSERT(suite, expr) (!(expr) ? test_fail(test_printf(suite, "ASSERT(%s) failed (%s:%d)", #expr, __FILE__, __LINE__)) : test_pass()) */
#define ASSERT_MSG(suite, expr, error) (!(expr) ? test_fail(suite, error) : test_pass(suite))

#endif /* TEST_H */
