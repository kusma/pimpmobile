#include "serializer.h"

#include <stdio.h>
#include <string.h>

#include <assert.h>
#define ASSERT assert

#if 0
#define TRACE() printf("AT %s:%d\n", __FILE__, __LINE__);
#else
#define TRACE()
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

/* TODO: get rid of this C++ dependency */
#include <map>
std::multimap<void *, unsigned> pointer_map;
std::map<void *, unsigned> pointer_back_map;

void serializer_init(struct serializer *s)
{
	ASSERT(NULL != s);
	TRACE();

	s->data = NULL;
	s->buffer_size = 0;
	s->pos = 0;
	
	pointer_map.clear();
	pointer_back_map.clear();
}

void serializer_deinit(struct serializer *s)
{
	ASSERT(NULL != s);
	TRACE();
	
	if (NULL != s->data)
	{
		free(s->data);
		s->data = NULL;
	}
}

void serializer_check_size(struct serializer *s, size_t needed_size)
{
	ASSERT(NULL != s);
	TRACE();
	
	if (s->buffer_size < (s->pos + needed_size))
	{
		int old_size = s->buffer_size;
		TRACE();
		s->buffer_size = MAX(s->buffer_size * 2, s->buffer_size + needed_size);

		s->data = (unsigned char*)realloc(s->data, s->buffer_size);
		
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
	TRACE();
	
	if ((s->pos % alignment) != 0)
	{
		int align_amt = alignment - (s->pos % alignment);
		serializer_check_size(s, align_amt);
		s->pos += align_amt;
	}
}

void serialize_byte(struct serializer *s, unsigned char b)
{
	ASSERT(NULL != s);
	TRACE();
	
	/* no alignment needed, make room for new data */
	serializer_check_size(s, 1);
	
	ASSERT(s->pos < s->buffer_size);
	s->data[s->pos++] = b;
}

void serialize_halfword(struct serializer *s, unsigned short h)
{
	ASSERT(NULL != s);
	TRACE();
	
	/* align and make room for new data */
	serializer_align(s, 2);
	serializer_check_size(s, 2);
	
	/* write data (little endian) */
	s->data[s->pos++] = (unsigned char)(h >> 0);
	s->data[s->pos++] = (unsigned char)(h >> 8);
}

void serialize_word(struct serializer *s, unsigned int w)
{
	ASSERT(NULL != s);
	TRACE();
	
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
	TRACE();
	
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
	int iptr;
	ASSERT(NULL != s);
	TRACE();
	
	serializer_align(s, 4);
	serializer_check_size(s, 4);
	
/*	printf("dumping ptr: %p\n", ptr); */
	if (NULL != ptr) pointer_map.insert(std::make_pair(ptr, s->pos));
	
/*	iptr = ptr & ((1ULL<<32) - 1);
	ASSERT(ptr == iptr); */
	serialize_word(s, (unsigned int)ptr);
}

void serializer_set_pointer(struct serializer *s, void *ptr, int pos)
{
	ASSERT(NULL != s);
	TRACE();
	
	pointer_back_map.insert(std::make_pair(ptr, pos));
}


void serializer_fixup_pointers(struct serializer *s)
{
	ASSERT(NULL != s);
	TRACE();
	
	std::multimap<void *, unsigned>::iterator it;
	
	for (it = pointer_map.begin(); it != pointer_map.end(); ++it)
	{
		ASSERT(pointer_back_map.count(it->first) != 0);
		
		unsigned *target = (unsigned*)(&s->data[it->second]);
		
		ASSERT(*target == (unsigned)it->first);
		
		*target = pointer_back_map[it->first] - it->second;
	}
}

