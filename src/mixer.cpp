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

s32 event_delta = 0;
s32 event_cursor = 0;

bool detect_loop_event(channel_t &chan, size_t samples)
{
	assert(samples != 0);
	
	s32 check_point;
	s32 end_sample = chan.sample_cursor + chan.sample_cursor_delta * samples;
	
	switch (chan.sample->loop_type)
	{
	case LOOP_TYPE_NONE:
		check_point = chan.sample->len;
	break;
	case LOOP_TYPE_FORWARD:
		check_point = chan.sample->loop_end;
	break;
	case LOOP_TYPE_PINGPONG:
		if (chan.sample_cursor_delta >= 0)
		{
			// moving forwards through the sample
			check_point = chan.sample->loop_end;
		}
		else
		{
			// moving backwards through the sample
			check_point = chan.sample->loop_start;
			if (end_sample < (check_point << 12))
			{
				event_delta = -chan.sample_cursor_delta;
				event_cursor = -((check_point << 12) - chan.sample_cursor);
				return true;
			}
			else return false;
		}
	break;
	}
	
	if (end_sample >= (check_point << 12))
	{
		event_delta = chan.sample_cursor_delta;
		event_cursor = (check_point << 12) - chan.sample_cursor;
		return true;
	}
	
	return false;
}

// returns false if we hit sample-end
bool process_loop_event(channel_t &chan)
{
	switch (chan.sample->loop_type)
	{
	case LOOP_TYPE_NONE:
		return false;
	break;
	case LOOP_TYPE_FORWARD:
		do
		{
			chan.sample_cursor -= (chan.sample->loop_end - chan.sample->loop_start) << 12;
		}
		while (chan.sample_cursor >= (chan.sample->loop_end << 12));
	break;
	case LOOP_TYPE_PINGPONG:
		do
		{
			if (chan.sample_cursor_delta >= 0)
			{
				chan.sample_cursor -= chan.sample->loop_end << 12;
				chan.sample_cursor = -chan.sample_cursor;
				chan.sample_cursor += chan.sample->loop_end << 12;
			}
			else
			{
				chan.sample_cursor -= chan.sample->loop_start << 12;
				chan.sample_cursor = -chan.sample_cursor;
				chan.sample_cursor += chan.sample->loop_start << 12;
			}
			chan.sample_cursor_delta = -chan.sample_cursor_delta;
		}
		while (chan.sample_cursor > (chan.sample->loop_end << 12) || chan.sample_cursor < (chan.sample->loop_start << 12));
	break;
	}

	return true;
}

u32 dc_offs = 0;
static inline void mix_channel(channel_t &chan, s32 *target, size_t samples)
{
	dc_offs += chan.volume * 128;

	assert(samples > 0);
	while (samples > 0 && detect_loop_event(chan, samples) == true)
	{
		BG_COLORS[0] = RGB5(0, 31, 0);
		do
		{
			assert((chan.sample_cursor >> 12) < chan.sample->len);
			
			s32 samp = ((u8*)chan.sample->data)[chan.sample_cursor >> 12];
			chan.sample_cursor += chan.sample_cursor_delta;
			*target++ += samp * chan.volume;
			
			samples--;
			event_cursor -= event_delta;
		} while (event_cursor > 0);
		
		assert(samples >= 0);
		
		if (process_loop_event(chan) == false)
		{
			while (samples--)
			{
				*target++ += chan.volume * 128;
			}
			
			chan.sample = 0;
			return;
		}
	}
	
	BG_COLORS[0] = RGB5(31, 0, 31);
	
	register const u8 *const sample_data = (const u8 *const)chan.sample->data;
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

	dc_offs = 0;
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		channel_t &chan = channels[c];
		if (0 != chan.sample) mix_channel(chan, sound_mix_buffer, samples);
	}
	dc_offs >>= 8;

	BG_COLORS[0] = RGB5(0, 0, 31);
	
	register s32 *src = sound_mix_buffer;
	register s8  *dst = target;
	register u32 dc_offs_local = dc_offs;
	
	// the compiler is too smart -- we need to prevent it from doing some arm11-optimizations.
	register s32 high_clamp = 127 + dc_offs;
	register s32 low_clamp = -128 + dc_offs;
	
#define ITERATION                                         \
	{						  \
		s32 samp = *src++ >> 8;                   \
		if (samp > high_clamp) samp = high_clamp; \
		if (samp < low_clamp) samp = low_clamp;   \
		samp -= dc_offs_local;                    \
		*dst++ = samp;                            \
	}
	
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
#undef ITERATION

	BG_COLORS[0] = RGB5(0, 0, 0);
}
