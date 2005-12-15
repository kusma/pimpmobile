#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "converter.h"

unsigned buffer_size = 0;
unsigned char *data = 0;

unsigned pos = 0;
void print_datastruct(const char *format, ...)
{
	va_list marker;
	va_start(marker, format);
	
	unsigned int i;

	while (*format != '\0')
	{
		if (buffer_size < (pos + 4))
		{
			buffer_size *= 2;
			data = (unsigned char*)realloc(data, buffer_size);
			memset(&data[buffer_size / 2], 0, buffer_size / 2);
		}
		
		switch (*format++)
		{
			case 'x':
				pos++;
			break;
			
			case 'b':
				i = va_arg(marker, int);
				data[pos + 0] = (unsigned char)(i >> 0);
				pos++;
			break;
			
			case 'h':
				i = va_arg(marker, int);
				if ((pos % 2) != 0) printf("warning: halfword not aligned\n");
				data[pos + 0] = (unsigned char)(i >> 0);
				data[pos + 1] = (unsigned char)(i >> 8);
				pos += 2;
			break;
			
			case 'i':
				i = va_arg(marker, int);
				if ((pos % 4) != 0) printf("warning: integer not aligned\n");
				data[pos + 0] = (unsigned char)(i >> 0);
				data[pos + 1] = (unsigned char)(i >> 8);
				data[pos + 2] = (unsigned char)(i >> 16);
				data[pos + 3] = (unsigned char)(i >> 24);
				pos += 4;
			break;
			
			default: assert(0);
		}
	}
	va_end(marker);
}


void dump_module(module_t *mod)
{
	printf("\ndumping module \"%s\"\n\n", mod->name);
	pos = 0;
	buffer_size = 1024;

	data = (unsigned char*)malloc(buffer_size);
	memset(data, 0, buffer_size);
	
	print_datastruct("ixxhx", 0xDEADBABE, 0xF00D);
	print_datastruct("ixxhx", 0xDEADBABE, 0xF00D);
	print_datastruct("ixxhx", 0xDEADBABE, 0xF00D);
	printf("\n");
	
	for (unsigned i = 0; i < pos; ++i)
	{
		printf("%02X ", data[i]);
		if ((i % 16) == 15) printf("\n");
	}
	
	free(data);
}
