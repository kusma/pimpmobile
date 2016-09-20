/* serializer.c -- low-level serializer for pimpconv
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include "serializer.h"

#include <stdio.h>
#include <string.h>

#include <assert.h>
#define ASSERT assert

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

void serializer_init(struct serializer *s)
{
	ASSERT(NULL != s);

	s->data = NULL;
	s->buffer_size = 0;
	s->pos = 0;

	s->relocs = NULL;
	s->num_relocs = 0;
}

void serializer_deinit(struct serializer *s)
{
	ASSERT(NULL != s);
	
	if (NULL != s->data)
	{
		free(s->data);
		s->data = NULL;
	}
}

void serializer_check_size(struct serializer *s, size_t needed_size)
{
	ASSERT(NULL != s);
	
	if (s->buffer_size < (s->pos + needed_size))
	{
		int old_size = s->buffer_size;
		s->buffer_size = MAX(s->buffer_size * 2, s->buffer_size + needed_size);

		s->data = realloc(s->data, s->buffer_size);
		
		if (NULL == s->data)
		{
			fputs("out of memory\n", stderr);
			abort();
		}
		
		memset(&s->data[old_size], 0, (s->buffer_size - old_size));
	}
}

void serializer_align(struct serializer *s, int alignment)
{
	ASSERT(NULL != s);
	
	if ((s->pos % alignment) != 0)
	{
		int align_amt = alignment - (s->pos % alignment);
		serializer_check_size(s, align_amt);
		s->pos += align_amt;
	}
}

void serialize_byte(struct serializer *s, uint8_t b)
{
	ASSERT(NULL != s);
	
	/* no alignment needed, make room for new data */
	serializer_check_size(s, 1);
	
	ASSERT(s->pos < s->buffer_size);
	s->data[s->pos++] = b;
}

void serialize_halfword(struct serializer *s, uint16_t h)
{
	ASSERT(NULL != s);
	
	/* align and make room for new data */
	serializer_align(s, 2);
	serializer_check_size(s, 2);
	
	/* write data (little endian) */
	s->data[s->pos++] = (unsigned char)(h >> 0);
	s->data[s->pos++] = (unsigned char)(h >> 8);
}

void serialize_word(struct serializer *s, uint32_t w)
{
	ASSERT(NULL != s);
	
	/* align and make room for new data */
	serializer_align(s, 4);
	serializer_check_size(s, 4);

	/* write data (little endian) */
	s->data[s->pos++] = (unsigned char)(w >> 0);
	s->data[s->pos++] = (unsigned char)(w >> 8);
	s->data[s->pos++] = (unsigned char)(w >> 16);
	s->data[s->pos++] = (unsigned char)(w >> 24);
}

/* why on earth does this routine support zero len? */
void serialize_string(struct serializer *s, const char *str, const size_t len)
{
	size_t real_len, slen;

	ASSERT(NULL != s);
	
	/* no alignment needed */
	serializer_check_size(s, len);

	/* determine length (min of slen and len) */
	slen = strlen(str) + 1;
	if (len > 0) real_len = (len > slen) ? slen : len;
	else real_len = slen;
	
	for (int i = 0; i < real_len; ++i)
	{
		s->data[s->pos + i] = str[i];
	}
	
	if (len > 0) s->pos += len;
	else s->pos += slen;
}

void serialize_pointer(struct serializer *s, void *ptr)
{
	ASSERT(NULL != s);
	
	serializer_align(s, 4);
	serializer_check_size(s, 4);

	if (!ptr) {
		serialize_word(s, 0);
		return;
	}

	s->relocs = realloc(s->relocs, sizeof(struct reloc) * (s->num_relocs + 1));
	if (!s->relocs) {
		fputs("out of memory\n", stderr);
		abort();
	}

	s->relocs[s->num_relocs].pos = s->pos;
	s->relocs[s->num_relocs].ptr = ptr;
	s->num_relocs++;

	serialize_word(s, 0xdeadbeef);
}

void serializer_set_pointer(struct serializer *s, void *ptr, int pos)
{
	ASSERT(NULL != s);

	for (int i = 0; i < s->num_relocs; ++i) {
		unsigned int *target;

		if (s->relocs[i].ptr != ptr)
			continue;

		ASSERT(s->relocs[i].pos & 3 == 0); /* location muet be word-aligned */

		target = (unsigned int*)(s->data + s->relocs[i].pos);
		ASSERT(*target == 0xdeadbeef);

		*target = pos - s->relocs[i].pos;
	}
}


void serializer_fixup_pointers(struct serializer *s)
{
	ASSERT(NULL != s);

	for (int i = 0; i < s->num_relocs; ++i) {
		unsigned int *target = (unsigned int*)(s->data + s->relocs[i].pos);
		if (*target == 0xdeadbeef) {
			fputs("reloc not fixed up!\n", stderr);
			abort();
		}
	}
}
