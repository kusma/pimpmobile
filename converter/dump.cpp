#include "dump.h"

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

static unsigned buffer_size = 0;
static unsigned char *data = 0;
static unsigned pos = 0;


#include <map>
std::multimap<void *, unsigned> pointer_map;
std::map<void *, unsigned> pointer_back_map;


void dump_init()
{
	free(data);
	data = NULL;
	buffer_size = 0;
	pos = 0;
	pointer_map.clear();
	pointer_back_map.clear();
}

void check_size(int needed_size)
{
	TRACE();
	if (buffer_size < (pos + needed_size))
	{
		int old_size = buffer_size;
		TRACE();
		buffer_size = MAX(buffer_size * 2, buffer_size + needed_size);

//		printf("old: %p %d\n", data, buffer_size);
		data = (unsigned char*)realloc(data, buffer_size);
//		printf("new %p\n", data);
		
		if (NULL == data)
		{
			printf("out of memory(?)\n");
			exit(1);
		}
		
		memset(&data[old_size], 0, (buffer_size - old_size));
	}
}

void align(int alignment)
{
	TRACE();
	if ((pos % alignment) != 0)
	{
		int align_amt = alignment - (pos % alignment);
		check_size(align_amt);
		pos += align_amt;
	}
}

int  get_pos()
{
	return pos;
}

const void *get_data()
{
	return data;
}


void dump_byte(unsigned char b)
{
	TRACE();
	check_size(1);
	ASSERT(pos < buffer_size);
	data[pos + 0] = b;
	pos++;
}

void dump_halfword(unsigned short h)
{
	TRACE();
	align(2);
	check_size(2);
	pos += pos % 2;
	data[pos + 0] = (unsigned char)(h >> 0);
	data[pos + 1] = (unsigned char)(h >> 8);
	pos += 2;
}

void dump_word(unsigned int w)
{
	TRACE();
	align(4);
	check_size(4);
	pos += pos % 4;
	data[pos + 0] = (unsigned char)(w >> 0);
	data[pos + 1] = (unsigned char)(w >> 8);
	data[pos + 2] = (unsigned char)(w >> 16);
	data[pos + 3] = (unsigned char)(w >> 24);
	pos += 4;
}

void dump_string(const char *str, const size_t len)
{
	size_t slen = strlen(str) + 1;
	size_t real_len;

	TRACE();

	if (len > 0) real_len = (len > slen) ? slen : len;
	else real_len = slen;
	
	for (int i = 0; i < real_len; ++i)
	{
		data[pos + i] = str[i];
	}
	
	if (len > 0) pos += len;
	else pos += slen;
}

void dump_pointer(void *ptr)
{
	TRACE();
	
	align(4);
	check_size(4);
	pointer_map.insert(std::make_pair(ptr, pos));
	
	dump_word((unsigned int)ptr);
}

void set_pointer(void *ptr, int pos)
{
	TRACE();
	pointer_back_map.insert(std::make_pair(ptr, pos));	
}


void fixup_pointers()
{
	TRACE();
	
	std::multimap<void *, unsigned>::iterator it;
	
	for (it = pointer_map.begin(); it != pointer_map.end(); ++it)
	{
		if (pointer_back_map.count(it->first) == 0)
		{
			printf("%p refered to, but not set\n", it->first);
			exit(1);
		}
		
		unsigned *target = (unsigned*)(&data[it->second]);
		
		// verify that the data pointed to is really the original pointer (just another sanity-check)
		if (*target != (unsigned)it->first)
		{
			printf("POOOTATOOOOO!\n");
			exit(1);
		}
		
		*target = pointer_back_map[it->first] - it->second;
	}
}
