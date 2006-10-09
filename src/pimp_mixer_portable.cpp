#include <stdlib.h>
#include <assert.h>
#include "pimp_mixer.h"

u32 __pimp_mixer_mix_samples(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
	assert(target != 0);
	assert(sample_data != 0);

	for (unsigned i = samples; i; --i)
	{
		register s32 samp = sample_data[sample_cursor >> 12];
		sample_cursor += sample_cursor_delta;
		*target++ += samp * vol;
		samples--;
	}
	
	return sample_cursor;
}

void __pimp_mixer_clip_samples(s8 *target, s32 *source, u32 samples, u32 dc_offs)
{
	s32 high_clamp = 127 + dc_offs;
	s32 low_clamp = -128 + dc_offs;
	
	assert(target != NULL);
	assert(source != NULL);
	
	for (unsigned i = samples; i; --i)
	{
		s32 samp = *source++;
		samp -= dc_offs << 8;
		samp >>= 6;
//		s32 samp = (*source++) >> 8;
//		samp -= dc_offs;
		if (samp >  127) samp =  127;
		if (samp < -128) samp = -128;
		*target++ = samp;
	}
}
