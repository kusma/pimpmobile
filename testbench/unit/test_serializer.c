#include "../framework/test_framework.h"
#include "../framework/test_helpers.h"

#include "../../converter/serializer.h"

static void test_serializer_basic(struct test_suite *suite)
{
	struct serializer s;
	
	serializer_init(&s);
	
	serialize_byte(&s, 0xde);
	serialize_byte(&s, 0xad);
	serialize_byte(&s, 0xbe);
	serialize_byte(&s, 0xef);
	
	ASSERT_INTS_EQUAL(suite, s.data[0], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[1], 0xad);
	ASSERT_INTS_EQUAL(suite, s.data[2], 0xbe);
	ASSERT_INTS_EQUAL(suite, s.data[3], 0xef);
	
	serializer_deinit(&s);
}

static void test_serializer_endianess(struct test_suite *suite)
{
	struct serializer s;
	
	serializer_init(&s);
	
	
	/* assure that 32bit words are dumped in little endian format */
	serialize_word(&s, 0xdeadbeef);
	ASSERT_INTS_EQUAL(suite, s.data[3], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[2], 0xad);
	ASSERT_INTS_EQUAL(suite, s.data[1], 0xbe);
	ASSERT_INTS_EQUAL(suite, s.data[0], 0xef);

	/* assure that 16bit halfwords are dumped in little endian format */
	serialize_halfword(&s, 0xb00b);
	ASSERT_INTS_EQUAL(suite, s.data[5], 0xb0);
	ASSERT_INTS_EQUAL(suite, s.data[4], 0x0b);
	
	serializer_deinit(&s);
}

static void test_serializer_align_word(struct test_suite *suite)
{
	struct serializer s;
	
	serializer_init(&s);
	
	/* 0 bytes offset */
	serialize_word(&s, 0xdeadbeef);
	ASSERT_INTS_EQUAL(suite, s.data[3], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[2], 0xad);
	ASSERT_INTS_EQUAL(suite, s.data[1], 0xbe);
	ASSERT_INTS_EQUAL(suite, s.data[0], 0xef);

	/* 1 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_word(&s, 0xdeadbeef);
	ASSERT_INTS_EQUAL(suite, s.data[11], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[10], 0xad);
	ASSERT_INTS_EQUAL(suite, s.data[9], 0xbe);
	ASSERT_INTS_EQUAL(suite, s.data[8], 0xef);

	/* 2 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_word(&s, 0xdeadbeef);
	ASSERT_INTS_EQUAL(suite, s.data[19], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[18], 0xad);
	ASSERT_INTS_EQUAL(suite, s.data[17], 0xbe);
	ASSERT_INTS_EQUAL(suite, s.data[16], 0xef);
	
	/* 3 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_word(&s, 0xdeadbeef);
	ASSERT_INTS_EQUAL(suite, s.data[27], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[26], 0xad);
	ASSERT_INTS_EQUAL(suite, s.data[25], 0xbe);
	ASSERT_INTS_EQUAL(suite, s.data[24], 0xef);
	
	/* 4 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_word(&s, 0xdeadbeef);
	ASSERT_INTS_EQUAL(suite, s.data[35], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[34], 0xad);
	ASSERT_INTS_EQUAL(suite, s.data[33], 0xbe);
	ASSERT_INTS_EQUAL(suite, s.data[32], 0xef);
	
	serializer_deinit(&s);
}

static void test_serializer_align_halfword(struct test_suite *suite)
{
	struct serializer s;
	
	serializer_init(&s);
	
	/* 0 bytes offset */
	serialize_halfword(&s, 0xdead);
	ASSERT_INTS_EQUAL(suite, s.data[1], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[0], 0xad);

	/* 1 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_halfword(&s, 0xdead);
	ASSERT_INTS_EQUAL(suite, s.data[5], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[4], 0xad);
	
	/* 2 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_halfword(&s, 0xdead);
	ASSERT_INTS_EQUAL(suite, s.data[9], 0xde);
	ASSERT_INTS_EQUAL(suite, s.data[8], 0xad);
	
	serializer_deinit(&s);
}


static void test_serializer_find(struct test_suite *suite)
{
	const unsigned char data[4] = { 0xde, 0xad, 0xbe, 0xef };
	
	struct serializer s;
	serializer_init(&s);
		
	serialize_byte(&s, 0xFF);
	
	/* assure that 32bit words are dumped in little endian format */
	serialize_byte(&s, data[0]);
	serialize_byte(&s, data[1]);
	serialize_byte(&s, data[2]);
	ASSERT_INTS_EQUAL(suite, serializer_find_data(&s, data, 4), -1);
	
	serialize_byte(&s, data[3]);
	ASSERT_INTS_EQUAL(suite, serializer_find_data(&s, data, 4), 1);
	
	serializer_deinit(&s);
}

void test_serializer(struct test_suite *suite)
{
	test_serializer_basic(suite);
	test_serializer_endianess(suite);
	test_serializer_align_word(suite);
	test_serializer_align_halfword(suite);
	test_serializer_find(suite);
}
