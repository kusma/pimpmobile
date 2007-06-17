#include "../framework/test.h"
#include "../framework/test_helpers.h"
#include "../../converter/serializer.h"

static void test_serializer_basic(void)
{
	struct serializer s;
	
	serializer_init(&s);
	
	serialize_byte(&s, 0xde);
	serialize_byte(&s, 0xad);
	serialize_byte(&s, 0xbe);
	serialize_byte(&s, 0xef);
	
	TEST_INTS_EQUAL(s.data[0], 0xde);
	TEST_INTS_EQUAL(s.data[1], 0xad);
	TEST_INTS_EQUAL(s.data[2], 0xbe);
	TEST_INTS_EQUAL(s.data[3], 0xef);
	
	serializer_deinit(&s);
}

static void test_serializer_endianess(void)
{
	struct serializer s;
	
	serializer_init(&s);
	
	
	/* assure that 32bit words are dumped in little endian format */
	serialize_word(&s, 0xdeadbeef);
	TEST_INTS_EQUAL(s.data[3], 0xde);
	TEST_INTS_EQUAL(s.data[2], 0xad);
	TEST_INTS_EQUAL(s.data[1], 0xbe);
	TEST_INTS_EQUAL(s.data[0], 0xef);

	/* assure that 16bit halfwords are dumped in little endian format */
	serialize_halfword(&s, 0xb00b);
	TEST_INTS_EQUAL(s.data[5], 0xb0);
	TEST_INTS_EQUAL(s.data[4], 0x0b);
	
	serializer_deinit(&s);
}

static void test_serializer_align_word(void)
{
	struct serializer s;
	
	serializer_init(&s);
	
	/* 0 bytes offset */
	serialize_word(&s, 0xdeadbeef);
	TEST_INTS_EQUAL(s.data[3], 0xde);
	TEST_INTS_EQUAL(s.data[2], 0xad);
	TEST_INTS_EQUAL(s.data[1], 0xbe);
	TEST_INTS_EQUAL(s.data[0], 0xef);

	/* 1 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_word(&s, 0xdeadbeef);
	TEST_INTS_EQUAL(s.data[11], 0xde);
	TEST_INTS_EQUAL(s.data[10], 0xad);
	TEST_INTS_EQUAL(s.data[9], 0xbe);
	TEST_INTS_EQUAL(s.data[8], 0xef);

	/* 2 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_word(&s, 0xdeadbeef);
	TEST_INTS_EQUAL(s.data[19], 0xde);
	TEST_INTS_EQUAL(s.data[18], 0xad);
	TEST_INTS_EQUAL(s.data[17], 0xbe);
	TEST_INTS_EQUAL(s.data[16], 0xef);
	
	/* 3 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_word(&s, 0xdeadbeef);
	TEST_INTS_EQUAL(s.data[27], 0xde);
	TEST_INTS_EQUAL(s.data[26], 0xad);
	TEST_INTS_EQUAL(s.data[25], 0xbe);
	TEST_INTS_EQUAL(s.data[24], 0xef);
	
	/* 4 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_word(&s, 0xdeadbeef);
	TEST_INTS_EQUAL(s.data[35], 0xde);
	TEST_INTS_EQUAL(s.data[34], 0xad);
	TEST_INTS_EQUAL(s.data[33], 0xbe);
	TEST_INTS_EQUAL(s.data[32], 0xef);
	
	serializer_deinit(&s);
}

static void test_serializer_align_halfword(void)
{
	struct serializer s;
	
	serializer_init(&s);
	
	/* 0 bytes offset */
	serialize_halfword(&s, 0xdead);
	TEST_INTS_EQUAL(s.data[1], 0xde);
	TEST_INTS_EQUAL(s.data[0], 0xad);

	/* 1 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_halfword(&s, 0xdead);
	TEST_INTS_EQUAL(s.data[5], 0xde);
	TEST_INTS_EQUAL(s.data[4], 0xad);
	
	/* 2 bytes offset */
	serialize_byte(&s, 0x00);
	serialize_byte(&s, 0x00);
	serialize_halfword(&s, 0xdead);
	TEST_INTS_EQUAL(s.data[9], 0xde);
	TEST_INTS_EQUAL(s.data[8], 0xad);
	
	serializer_deinit(&s);
}


static void test_serializer_find(void)
{
	struct serializer s;
	serializer_init(&s);
	
	const unsigned char data[4] = { 0xde, 0xad, 0xbe, 0xef };
	
	serialize_byte(&s, 0xFF);
	
	/* assure that 32bit words are dumped in little endian format */
	serialize_byte(&s, data[0]);
	serialize_byte(&s, data[1]);
	serialize_byte(&s, data[2]);
	TEST_INTS_EQUAL(serializer_find_data(&s, data, 4), -1);
	
	serialize_byte(&s, data[3]);
	TEST_INTS_EQUAL(serializer_find_data(&s, data, 4), 1);
	
	serializer_deinit(&s);
}

void test_serializer(void)
{
	test_serializer_basic();
	test_serializer_endianess();
	test_serializer_align_word();
	test_serializer_align_halfword();
	test_serializer_find();
}
