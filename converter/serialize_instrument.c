#include <stdio.h>
#include "../src/pimp_module.h"
#include "../src/pimp_mixer.h"
#include "serializer.h"

#if 0
#include <stdio.h>
#define TRACE() printf("AT %s:%d\n", __FILE__, __LINE__);
#else
#define TRACE()
#endif

int serialize_sample(struct serializer *s, const pimp_sample *samp)
{
	int pos;
	
	ASSERT(NULL != samp);
	ASSERT(NULL != s);
	TRACE();
	
	serializer_align(s, 4);
	pos = s->pos;
	
	serialize_word(s, samp->data_ptr);
	serialize_word(s, samp->length);
	serialize_word(s, samp->loop_start);
	serialize_word(s, samp->loop_length);
	
	serialize_halfword(s, samp->fine_tune);
	serialize_halfword(s, samp->rel_note);
	
	serialize_byte(s, samp->volume);
	serialize_byte(s, samp->loop_type);
	serialize_byte(s, samp->pan);
	
	serialize_byte(s, samp->vibrato_speed);
	serialize_byte(s, samp->vibrato_depth);
	serialize_byte(s, samp->vibrato_sweep);
	serialize_byte(s, samp->vibrato_waveform);
	
	return pos;
}

int serialize_envelope(struct serializer *s, const pimp_envelope *env)
{
	int pos, i;
	
	ASSERT(NULL != env);
	ASSERT(NULL != s);
	TRACE();
	
	serializer_align(s, 4);
	pos = s->pos;
	
	for (i = 0; i < 25; ++i)
	{
		serialize_halfword(s, env->node_tick[i]);
	}
	
	for (i = 0; i < 25; ++i)
	{
		serialize_halfword(s, env->node_magnitude[i]);
	}
	
	for (i = 0; i < 25; ++i)
	{
		serialize_halfword(s, env->node_delta[i]);
	}
	
	serialize_byte(s, env->node_count);
	serialize_byte(s, env->flags);
	
	serialize_byte(s, env->loop_start);
	serialize_byte(s, env->loop_end);
	
	serialize_byte(s, env->sustain_loop_start);
	serialize_byte(s, env->sustain_loop_end);
	
	return pos;
}

/* dumps all samples in an instrument, returns position of first sample */
void serialize_instrument_data(struct serializer *s, const pimp_instrument *instr)
{
	int i;
	pimp_envelope *vol_env, *pan_env;
	
	ASSERT(NULL != instr);
	ASSERT(NULL != s);
	TRACE();
	
	serializer_align(s, 4);
	serializer_set_pointer(s, pimp_get_ptr(&instr->sample_ptr), s->pos);
	
	for (i = 0; i < instr->sample_count; ++i)
	{
		serialize_sample(s, pimp_instrument_get_sample(instr, i));
	}
	
	vol_env = pimp_instrument_get_vol_env(instr);
	if (NULL != vol_env) serializer_set_pointer(s, pimp_get_ptr(&instr->vol_env_ptr), serialize_envelope(s, vol_env));
	
	pan_env = pimp_instrument_get_pan_env(instr);
	if (NULL != pan_env) serializer_set_pointer(s, pimp_get_ptr(&instr->pan_env_ptr), serialize_envelope(s, pan_env));
}

/* dump instrument structure, return position */
void serialize_instrument(struct serializer *s, const pimp_instrument *instr)
{
	int i;
	
	ASSERT(NULL != instr);
	ASSERT(NULL != s);
	TRACE();
	
	serializer_align(s, 4);
	serialize_pointer(s, pimp_get_ptr(&instr->sample_ptr));
	serialize_pointer(s, pimp_get_ptr(&instr->vol_env_ptr));
	serialize_pointer(s, pimp_get_ptr(&instr->pan_env_ptr));
	
	serialize_halfword(s, instr->volume_fadeout);
	serialize_byte(s, instr->sample_count);
	
	for (i = 0; i < 120; ++i)
	{
		serialize_byte(s, instr->sample_map[i]);
	}
}
