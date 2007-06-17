#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

void test_int_array(const int *array, const int *reference, int size, const char *file, int line);

#define TEST_INTS_EQUAL(value, expected) TEST((value) == (expected), test_printf("ints not equal, got %d - expected %d (%s:%d)", (int)(value), (int)(expected), __FILE__, __LINE__))
#define TEST_INT_ARRAYS_EQUAL(array, reference, size) test_int_array(array, reference, size, __FILE__, __LINE__)

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

#endif /* TEST_HELPERS_H */
