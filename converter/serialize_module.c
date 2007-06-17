#include <stdio.h>

#include "serializer.h"
#include "serialize_module.h"

#include "../src/pimp_module.h"
#include "../src/pimp_mixer.h"

#if 0
#include <stdio.h>
#define TRACE() printf("AT %s:%d\n", __FILE__, __LINE__);
#else
#define TRACE()
#endif

#include "serialize_instrument.h"

void serialize_module_struct(struct serializer *s, const pimp_module *mod)
{
	ASSERT(NULL != s);
	ASSERT(NULL != mod);
	TRACE();
	
	/* dump pimp_module structure */
	serialize_string(s, mod->name, 32);
	serialize_word(s, mod->flags);
	serialize_word(s, mod->reserved);
	
	serialize_pointer(s, pimp_get_ptr(&mod->order_ptr));
	serialize_pointer(s, pimp_get_ptr(&mod->pattern_ptr));
	serialize_pointer(s, pimp_get_ptr(&mod->channel_ptr));
	serialize_pointer(s, pimp_get_ptr(&mod->instrument_ptr));
	
	serialize_halfword(s, mod->period_low_clamp);
	serialize_halfword(s, mod->period_high_clamp);
	serialize_halfword(s, mod->order_count);
	
	serialize_byte(s, mod->order_repeat);
	serialize_byte(s, mod->volume);
	serialize_byte(s, mod->tempo);
	serialize_byte(s, mod->bpm);
	
	serialize_byte(s, mod->instrument_count);
	serialize_byte(s, mod->pattern_count);
	serialize_byte(s, mod->channel_count);
}

void serialize_channels(struct serializer *s, const pimp_module *mod)
{
	int i;
	
	ASSERT(NULL != s);
	ASSERT(NULL != mod);
	TRACE();
	
	/* dump channels (all bytes, no serializer_alignment required) */
	serializer_align(s, 4);
	serializer_set_pointer(s, pimp_get_ptr(&mod->channel_ptr), s->pos);
	for (i = 0; i < mod->channel_count; ++i)
	{
		const pimp_channel *chan = __pimp_module_get_channel(mod, i);
		serialize_byte(s, chan->pan);
		serialize_byte(s, chan->volume);
		serialize_byte(s, chan->mute);
	}
}

void serialize_instruments(struct serializer *s, const pimp_module *mod)
{
	int i;

	ASSERT(NULL != s);
	ASSERT(NULL != mod);
	TRACE();
	
	/* dump instrument structs */
	serializer_align(s, 4);
	serializer_set_pointer(s, pimp_get_ptr(&mod->instrument_ptr), s->pos);
	for (i = 0; i < mod->instrument_count; ++i)
	{
		const pimp_instrument *instr = __pimp_module_get_instrument(mod, i);
		serialize_instrument(s, instr);
	}
	
	/* dump instrument data (samples, envelopes etc) */
	for (i = 0; i < mod->instrument_count; ++i)
	{
		const pimp_instrument *instr = __pimp_module_get_instrument(mod, i);
		serialize_instrument_data(s, instr);
	}
}

void serialize_orders(struct serializer *s, const pimp_module *mod)
{
	int i;

	ASSERT(NULL != s);
	ASSERT(NULL != mod);
	TRACE();
	
	/* dump orders (all bytes, no serializer_alignment required) */
	serializer_set_pointer(s, pimp_get_ptr(&mod->order_ptr), s->pos);
	for (i = 0; i < mod->order_count; ++i)
	{
		serialize_byte(s, __pimp_module_get_order(mod, i));
	}
}

void serialize_patterns(struct serializer *s, const pimp_module *mod)
{
	int i;

	ASSERT(NULL != s);
	ASSERT(NULL != mod);
	TRACE();
	
	/* dump array of pimp_pattern structs */
	serializer_align(s, 4);
	serializer_set_pointer(s, pimp_get_ptr(&mod->pattern_ptr), s->pos);
	for (i = 0; i < mod->pattern_count; ++i)
	{
		const pimp_pattern *pattern = __pimp_module_get_pattern(mod, i);
		serializer_align(s, 4);
		
		serialize_pointer(s, pimp_get_ptr(&pattern->data_ptr)); // data_ptr
		serialize_halfword(s, pattern->row_count);
	}
	
	/* dump pattern data */
	for (i = 0; i < mod->pattern_count; ++i)
	{
		int j;
		const pimp_pattern *pattern = __pimp_module_get_pattern(mod, i);
		if (NULL == pattern) continue;
		
		const pimp_pattern_entry *pattern_data = __pimp_pattern_get_data(pattern);
		if (NULL == pattern_data) continue;
		
		serializer_set_pointer(s, pimp_get_ptr(&pattern->data_ptr), s->pos);
		
		/* write the actual pattern data data */
		for (j = 0; j < pattern->row_count * mod->channel_count; ++j)
		{
			const pimp_pattern_entry *pe = &pattern_data[j];
			serialize_byte(s, pe->note);
			serialize_byte(s, pe->instrument);
			serialize_byte(s, pe->volume_command);
			serialize_byte(s, pe->effect_byte);
			serialize_byte(s, pe->effect_parameter);
		}
	}
}

void serialize_module(struct serializer *s, const pimp_module *mod)
{
	ASSERT(NULL != s);
	ASSERT(NULL != mod);
	TRACE();
		
	serialize_module_struct(s, mod);
	serialize_orders(s, mod);
	serialize_patterns(s, mod);
	serialize_channels(s, mod);
	serialize_instruments(s, mod);
}

pimp_module *load_module_xm(FILE *fp);

#if 0

int main(int argc, char *argv[])
{
	int i;
	
#if 0
	/* todo: unit test this! */
	unsigned int test;
	set_ptr(&test, (void*)0xdeadbeef);
	printf("------ %p\n", get_ptr(&test));
	
	/* todo: ...and this */	
	pimp_sample samp;
	set_ptr(&instr.sample_ptr, &samp);
	pimp_sample *s = get_sample(&instr, 0);
	s->volume = 33;
	printf("**** %d\n", samp.volume);
#endif

	{
		const char *filename = "../example/data/dxn-oopk.xm...";
		FILE *fp = fopen(filename, "rb");
		if (!fp)
		{
			printf("file not found!\n");
			exit(1);
		}
		
		printf("loading %s\n", filename);
		pimp_module *mod2 = load_module_xm(fp);

		if (NULL == mod2)
		{
			printf("failed to load!\n");
			exit(1);
		}
		
		serialize_module(mod2);
	}

	const char *data = (const char*)get_data();
/*
	int cnt = 0;
	for (int i = 0; i < get_pos(); ++i)
	{
		printf("%02X ", ((int)data[i]) & 0xFF);
		cnt++;
		if (cnt == 16)
		{
			printf("\n");
			cnt = 0;
		}
	}
	printf("\n");
*/

	/* dump to file */
	FILE *fp = fopen("dump.bin", "wb");
	if (NULL != fp)
	{
		printf("writing dump...\n");
		fwrite(data, 1, get_pos(), fp);
		fclose(fp);
	}
	
	return 0;
}

#endif
