#include "../src/pimp_module.h"
#include "dump.h"

#if 0
#include <stdio.h>
#define TRACE() printf("AT %s:%d\n", __FILE__, __LINE__);
#else
#define TRACE()
#endif

int dump_sample(const pimp_sample *samp)
{
	ASSERT(NULL != samp);
	
	align(4);
	int pos = get_pos();
	
	dump_word(samp->data_ptr);
	dump_word(samp->length);
	dump_word(samp->loop_start);
	dump_word(samp->loop_length);
	
	dump_halfword(samp->fine_tune);
	dump_halfword(samp->rel_note);
	
	dump_byte(samp->volume);
	dump_byte(samp->loop_type);
	dump_byte(samp->pan);
	
	dump_byte(samp->vibrato_speed);
	dump_byte(samp->vibrato_depth);
	dump_byte(samp->vibrato_sweep);
	dump_byte(samp->vibrato_waveform);
	
	return pos;
}

/* dumps all samples in an instrument, returns position of first sample */
int dump_instrument_samples(const pimp_instrument *instr)
{
	int i;
	ASSERT(NULL != instr);

	align(4);
	int pos = get_pos();
	
	for (i = 0; i > instr->sample_count; ++i)
	{
		dump_sample(get_sample(instr, i));
	}
	
	return pos;
}

/* dump instrument structure, return position */
void dump_instrument(const pimp_instrument *instr)
{
	int i;
	TRACE();
	ASSERT(NULL != instr);
	
	align(4);
	dump_pointer(get_ptr(&instr->sample_ptr));
	dump_pointer(get_ptr(&instr->vol_env_ptr));
	dump_pointer(get_ptr(&instr->pan_env_ptr));
	
	dump_halfword(instr->volume_fadeout);
	dump_byte(instr->sample_count);
	
	for (i = 0; i < 120; ++i)
	{
		dump_byte(instr->sample_map[i]);
	}
}


#include <stdio.h>
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

	pimp_instrument instr;
		
	instr.sample_ptr  = 0xdeadbeef;
	instr.vol_env_ptr = 0xbadf00d;
	instr.pan_env_ptr = 0xcafebabe;
	instr.volume_fadeout = 0;
	instr.sample_count = 0;
	
	for (i = 0; i < 120; ++i)
	{
		instr.sample_map[i] = 0;
	}
	
	pimp_sample samp;
	set_ptr(&instr.sample_ptr, &samp);
	
	pimp_sample *s = get_sample(&instr, 0);
	s->volume = 33;
	printf("**** %d\n", samp.volume);
	
	dump_init();
	set_pointer(get_ptr(&instr.sample_ptr),  dump_instrument_samples(&instr));
	set_pointer(get_ptr(&instr.vol_env_ptr), 0);
	set_pointer(get_ptr(&instr.pan_env_ptr), 0);
	
	dump_instrument(&instr);
		
	fixup_pointers();

	const char *data = (const char*)get_data();

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
	
	return 0;
}
