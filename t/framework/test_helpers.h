#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "test_framework.h"
void test_int_array(struct test_suite *suite, const int *array, const int *reference, int size, const char *file, int line);

#define TEST_RUN(suite, func) test_run(suite, func, #func);

#define ASSERT_INTS_EQUAL(suite, value, expected)              ASSERT_MSG(suite, (value) == (expected), test_printf(suite, "ints not equal, got %d - expected %d (%s:%d)", (int)(value), (int)(expected), __FILE__, __LINE__))
#define ASSERT_INT_ARRAYS_EQUAL(suite, array, reference, size) test_int_array(suite, array, reference, size, __FILE__, __LINE__)

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

#endif /* TEST_HELPERS_H */
