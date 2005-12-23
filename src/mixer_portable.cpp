#include <stdlib.h>
#include <assert.h>
#include "mixer.h"

#include <gba_base.h>

u32 mixer::mix_samples(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
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
