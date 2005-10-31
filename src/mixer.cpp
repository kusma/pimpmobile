#include <stdlib.h>
#include <assert.h>
#include "mixer.h"

#include <gba_systemcalls.h>
#include <gba_video.h>
#include <gba_dma.h>

using namespace mixer;

/*

miksing:

for each chan
	if looping && loop event (sample-stop is loop event)
		event_cursor = event_point;
		do {
			do {
				mix sample
				event_cursor -= event_delta
			}
			while (event_cursor > 0)
			
			handle event
		}
		while (not all samples rendered)
		
		mix all samples in tick with loop-check
	else
		if (volume == max) mix_megafast()
		mix all samples (unroll, baby)
	end if
end for

*/

channel_t mixer::channels[CHANNELS] IWRAM_DATA;

#include <gba_console.h>
#include <stdio.h>

#ifdef UNSIGNED_SAMPLES
// if we're using unsigned samples, we need to offset the DC based on the volume of all channels
u32 dc_offs = 0;
#endif

static inline void mix_channel(channel_t &chan, s32 *target, size_t samples)
{
//	if (0 == chan.sample) return;
	
#ifdef UNSIGNED_SAMPLES
	dc_offs += chan.volume * 128;
#endif

#if 1
	assert(samples > 0);

	bool loop_event = false;

	s32 end_sample = chan.sample_cursor + chan.sample_cursor_delta * samples;

	s32 event_delta = 0;
	s32 event_cursor = 0;
	
	if (end_sample >= (chan.sample->len << 12))
	{
		loop_event = true;
		event_delta = chan.sample_cursor_delta;
		event_cursor = (chan.sample->len << 12) - chan.sample_cursor;
	}
	
	if (loop_event == true)
	{
		do
		{
			do
			{
				assert((chan.sample_cursor >> 12) < chan.sample->len);
				
				s32 samp = ((u8*)chan.sample->data)[chan.sample_cursor >> 12];
				chan.sample_cursor += chan.sample_cursor_delta;
				*target++ += samp * chan.volume;
				
				samples--;
				event_cursor -= event_delta;
			} while (event_cursor > 0);
			
			// TODO: check event-type
			while (samples--)
			{
				*target++ += chan.volume * 128;
			}
		
			chan.sample = 0;
			return;
		} while (--samples);
		return;
	}
#endif
	BG_COLORS[0] = RGB5(31, 0, 31);
	
#ifdef UNSIGNED_SAMPLES
	register const u8 *const sample_data = chan.sample->data;
#else
	register const s8 *const sample_data = chan.sample->data;
#endif
	
	register u32 sample_cursor = chan.sample_cursor;
	register const u32 sample_cursor_delta = chan.sample_cursor_delta;
	register const u32 vol = chan.volume;
	
#define ITERATION           \
	{                       \
		register s32 samp = sample_data[sample_cursor >> 12]; \
		sample_cursor += sample_cursor_delta; \
		*target++ += samp * vol;    \
	}
#if 1
	register u32 s = samples >> 4;
	switch (samples & 15)
	{
		do
		{
		ITERATION;
		case 15: ITERATION;
		case 14: ITERATION;
		case 13: ITERATION;
		case 12: ITERATION;
		case 11: ITERATION;
		case 10: ITERATION;
		case 9:  ITERATION;
		case 8:  ITERATION;
		case 7:  ITERATION;
		case 6:  ITERATION;
		case 5:  ITERATION;
		case 4:  ITERATION;
		case 3:  ITERATION;
		case 2:  ITERATION;
		case 1:  ITERATION;
		case 0:;
		}
		while (s--);
	}
#endif
	chan.sample_cursor = sample_cursor;

#undef ITERATION
	BG_COLORS[0] = RGB5(31, 0, 0);
}

void mixer::reset()
{
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		channels[c].sample = 0;
		channels[c].sample_cursor = 0;
		channels[c].sample_cursor_delta = 0;
	}
}

s32 sound_mix_buffer[SOUND_BUFFER_SIZE] IWRAM_DATA ALIGN(4);

void mixer::mix(s8 *target, size_t samples)
{
	assert(samples > 0);
	
	BG_COLORS[0] = RGB5(31, 0, 0);
	
	u32 zero = 0;
	CpuFastSet(&zero, sound_mix_buffer, DMA_SRC_FIXED | (samples));

	// CpuFastSet works on groups of 4 bytes, so the last byte might not be cleared.
	// unconditional clear is faster, so let's do it anyway
//	u32 zero = 0;
//	CpuFastSet(&zero, sound_mix_buffer, DMA_SRC_FIXED | (samples));
//	sound_mix_buffer[samples - 1] = 0;

#ifdef UNSIGNED_SAMPLES
	dc_offs = 0;
#endif
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		channel_t &chan = channels[c];
		if (0 != chan.sample) mix_channel(chan, sound_mix_buffer, samples);
	}
#ifdef UNSIGNED_SAMPLES
	dc_offs >>= 8;
#endif

	BG_COLORS[0] = RGB5(0, 0, 31);
	
	register s32 *src = sound_mix_buffer;
	register s8  *dst = target;

#ifdef UNSIGNED_SAMPLES
	register u32 dc_offs_local = dc_offs;
	// the compiler is too smart -- we need to prevent it from doing some arm11-optimizations.
	register s32 high_clamp = 127 + dc_offs;
	register s32 low_clamp = -128 + dc_offs;
#define ITERATION                                 \
	{						                      \
		s32 samp = *src++ >> 8;                   \
		if (samp > high_clamp) samp = high_clamp; \
		if (samp < low_clamp) samp = low_clamp;   \
		samp -= dc_offs_local;                    \
		*dst++ = samp;                            \
	}
#else
#define ITERATION                     \
	{						          \
		s32 samp = *src++ >> 8;       \
		if (samp > 127) samp = 127;   \
		if (samp < -128) samp = -128; \
		*dst++ = samp;                \
	}
#endif
	
	register u32 s = samples >> 4;
	switch (samples & 15)
	{
		do
		{
		ITERATION;
		case 15: ITERATION;
		case 14: ITERATION;
		case 13: ITERATION;
		case 12: ITERATION;
		case 11: ITERATION;
		case 10: ITERATION;
		case 9:  ITERATION;
		case 8:  ITERATION;
		case 7:  ITERATION;
		case 6:  ITERATION;
		case 5:  ITERATION;
		case 4:  ITERATION;
		case 3:  ITERATION;
		case 2:  ITERATION;
		case 1:  ITERATION;
		case 0:;
		
//		iprintf("ballesatan %i\n", chan.sample_cursor);
		}
		while (s--);
	}
#undef ITERATION

	BG_COLORS[0] = RGB5(0, 0, 0);
}
