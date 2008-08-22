/* pimp_mixer_portable.c -- A portable audio mixer
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include "pimp_base.h"
#include "pimp_mixer.h"
#include "pimp_debug.h"

void pimp_mixer_clear(s32 *target, u32 samples)
{
	int i;
	ASSERT(target != 0);
	
	for (i = 0; i < samples; ++i)
	{
		target[i] = 0;
	}
}

u32 pimp_mixer_mix_samples(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
	int i;
	ASSERT(target != 0);
	ASSERT(sample_data != 0);
	
	for (i = 0; i < samples; ++i)
	{
		s32 samp = sample_data[sample_cursor >> 12];
		sample_cursor += sample_cursor_delta;
		target[i] += samp * vol;
	}
	
	return sample_cursor;
}

void pimp_mixer_clip_samples(s8 *target, const s32 *source, u32 samples, u32 dc_offs)
{
	int i;
	ASSERT(target != NULL);
	ASSERT(source != NULL);
	
	for (i = 0; i < samples; ++i)
	{
		s32 samp = source[i];
		samp = (samp >> 8) - dc_offs;
		if (samp >  127) samp =  127;
		if (samp < -128) samp = -128;
		target[i] = samp;
	}
}
