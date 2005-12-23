#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "converter.h"
#include "../src/internal.h"

unsigned buffer_size = 0;
unsigned char *data = 0;

unsigned pos = 0;

void dump_byte(unsigned char b)
{
	data[pos + 0] = b;
	pos++;
}

void dump_halfword(unsigned short h)
{
	if ((pos % 2) != 0) printf("warning: halfword not aligned at pos %i, fixing\n", pos);
	
	// force alignment
	pos += pos % 2;
	data[pos + 0] = (unsigned char)(h >> 0);
	data[pos + 1] = (unsigned char)(h >> 8);
	pos += 2;
}

void dump_word(unsigned int w)
{
	if ((pos % 4) != 0) printf("warning: word not aligned at pos %i, fixing\n", pos);

	// force alignment
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

	if (len > 0) real_len = (len > slen) ? slen : len;
	else real_len = slen;
	
	for (int i = 0; i < real_len; ++i)
	{
		data[pos + i] = str[i];
	}
	
	if (len > 0) pos += len;
	else pos += slen;
	
	printf("%i\n", pos);
}

void dump_datastruct(const char *format, ...)
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
				dump_byte((unsigned char)i);
			break;
			
			case 'h':
				i = va_arg(marker, int);
				dump_halfword((unsigned short)i);
			break;
			
			case 'i':
				i = va_arg(marker, int);
				dump_word((unsigned int)i);
			break;
			
			default: assert(0);
		}
	}
	va_end(marker);
}

/*
struct foo
{
	int a __attribute__ ((aligned(8)));
	int b __attribute__ ((aligned(8)));
};
*/

#include <stddef.h>

/*
typedef struct
{
	char name[32];
	
	u16 period_low_clamp;
	u16 period_high_clamp;
	u16 order_length;
	
	u8  order_repeat;
	u8  volume;
	u8  tempo;
	u8  bpm;
	
	u8  instrument_count;
	u8  pattern_count;
	u8  channel_count;
	
	// these are offsets relative to the begining of the pimp_module_t-structure
	unsigned order_ptr;
	unsigned pattern_data_ptr;
	unsigned channel_ptr;
	unsigned instrument_ptr;
} pimp_module_t;
*/

void dump_module(module_t *mod)
{
//	printf("fjall: %i %i\n", offsetof(foo, a), offsetof(foo, b));

	printf("\ndumping module \"%s\"\n\n", mod->name);
	pos = 0;
	buffer_size = 1024;

	data = (unsigned char*)malloc(buffer_size);
	memset(data, 0, buffer_size);
	
	dump_string(mod->name, 32);

	unsigned flags = 0;
	if (mod->use_linear_frequency_table)   flags |= FLAG_LINEAR_PERIODS;
	if (mod->instrument_vibrato_use_linear_frequency_table) flags |= FLAG_LINEAR_VIBRATO;
	if (mod->volume_slide_in_tick0)        flags |= FLAG_VOL_SLIDE_TICK0;
	if (mod->vibrato_in_tick0)             flags |= FLAG_VIBRATO_TICK0;
	if (mod->vol0_optimizations)           flags |= FLAG_VOL0_OPTIMIZE;
	if (mod->tremor_extra_delay)           flags |= FLAG_TEMOR_EXTRA_DELAY;
	if (mod->tremor_has_memory)            flags |= FLAG_TEMOR_MEMORY;
	if (mod->retrig_kills_note)            flags |= FLAG_RETRIG_KILLS_NOTE;
	if (mod->note_cut_kills_note)          flags |= FLAG_NOTE_CUT_KILLS_NOTE;
	if (mod->allow_nested_loops)           flags |= FLAG_ALLOW_NESTED_LOOPS;
	if (mod->retrig_note_source_is_period) flags |= FLAG_RETRIG_NOTE_PERIOD;
	if (mod->delay_global_volume)          flags |= FLAG_DELAY_GLOBAL_VOLUME;
	if (mod->sample_offset_clamp)          flags |= FLAG_SAMPLE_OFFSET_CLAMP;
	if (mod->tone_porta_share_memory)      flags |= FLAG_PORTA_NOTE_SHARE_MEMORY;
	if (mod->remember_tone_porta_target)   flags |= FLAG_PORTA_NOTE_MEMORY;	
	dump_word(flags);

	dump_halfword(mod->period_low_clamp);
	dump_halfword(mod->period_high_clamp);
	dump_halfword(mod->play_order_length);
	dump_byte(mod->play_order_repeat_position);
	dump_byte(mod->initial_global_volume);
	dump_byte(mod->initial_tempo);
	dump_byte(mod->initial_bpm);
	
	dump_byte(mod->instrument_count);
	dump_byte(mod->pattern_count);
	dump_byte(mod->channel_count);

	
//	dump_datastruct("i", 0xFFFFFFFF);
	printf("\n");
	
	FILE *fp = fopen("out.bin", "wb");
	for (unsigned i = 0; i < pos; ++i)
	{
		fwrite(&data[i], 1, 1, fp);
		printf("%02X ", data[i]);
		if ((i % 16) == 15) printf("\n");
	}
	fclose(fp);

/*	
    NOT HERE! THIS IS A GLOBAL EXPORT!
	
	FILE *fp = fopen("samples.bin", "wb");
	for (unsigned i = 0; i < mod->instrument_count; ++i)
	{
		
	}
	fclose(fp);
*/
	
	free(data);
}
