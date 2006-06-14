#include <stdlib.h>
#include <assert.h>
#include "mixer.h"
#include "pimp_debug.h"

#include <gba_systemcalls.h>
#include <gba_dma.h>
#include <gba_timers.h>

using namespace mixer;

/*

mixing:

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
		mix all samples
	end if
end for

*/

volatile channel_t mixer::channels[CHANNELS] IWRAM_DATA;

#include <gba_console.h>
#include <stdio.h>

s32 event_delta = 0;
s32 event_cursor = 0;

bool detect_loop_event(channel_t &chan, size_t samples)
{
	assert(samples != 0);
	
	s32 check_point;
	s32 end_sample = chan.sample_cursor + chan.sample_cursor_delta * samples;
	
	switch (chan.loop_type)
	{
	case LOOP_TYPE_NONE:
		check_point = chan.sample_length;
	break;
	case LOOP_TYPE_FORWARD:
		check_point = chan.loop_end;
	break;
	case LOOP_TYPE_PINGPONG:
		if (chan.sample_cursor_delta >= 0)
		{
			// moving forwards through the sample
			check_point = chan.loop_end;
		}
		else
		{
			// moving backwards through the sample
			check_point = chan.loop_start;
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
	switch (chan.loop_type)
	{
	case LOOP_TYPE_NONE:
		return false;
	break;
	case LOOP_TYPE_FORWARD:
		do
		{
			chan.sample_cursor -= (chan.loop_end - chan.loop_start) << 12;
		}
		while (chan.sample_cursor >= (chan.loop_end << 12));
	break;
	case LOOP_TYPE_PINGPONG:
		do
		{
			if (chan.sample_cursor_delta >= 0)
			{
				chan.sample_cursor -= chan.loop_end << 12;
				chan.sample_cursor = -chan.sample_cursor;
				chan.sample_cursor += chan.loop_end << 12;
			}
			else
			{
				chan.sample_cursor -= chan.loop_start << 12;
				chan.sample_cursor = -chan.sample_cursor;
				chan.sample_cursor += chan.loop_start << 12;
			}
			chan.sample_cursor_delta = -chan.sample_cursor_delta;
		}
		while (chan.sample_cursor > (chan.loop_end << 12) || chan.sample_cursor < (chan.loop_start << 12));
	break;
	}

	return true;
}

inline void timing_start()
{
	REG_TM3CNT_H = 0;
	REG_TM3CNT_L = 0;
	REG_TM3CNT_H = TIMER_START;
}

inline void timing_end()
{
	unsigned int fjall = REG_TM3CNT_L;
//	iprintf("cycles pr sample: %i\n", fjall / SOUND_BUFFER_SIZE);
//	iprintf("%i per cent cpu\n", (fjall * 1000) / 280896);
}

u32 dc_offs = 0;
static inline void mix_channel(channel_t &chan, s32 *target, size_t samples)
{
	if (chan.volume < 1) return;
	dc_offs += chan.volume * 128;

	assert(samples > 0);
	
	while (samples > 0 && detect_loop_event(chan, samples) == true)
	{
		do
		{
			assert((chan.sample_cursor >> 12) < chan.sample_length);
			assert(chan.sample_data != 0);
			
			s32 samp = ((u8*)chan.sample_data)[chan.sample_cursor >> 12];
			chan.sample_cursor += chan.sample_cursor_delta;
			*target++ += samp * chan.volume;
			
			samples--;
			event_cursor -= event_delta;
		}
		while (event_cursor > 0);
		
		assert(samples >= 0);
		
		if (process_loop_event(chan) == false)
		{
			// the sample has stopped, we need to fill the rest of the buffer with the dc-offset, so it doesn't ruin our unsigned mixing-thing
			while (samples--)
			{
				*target++ += chan.volume * 128;
			}
			// terminate sample
			chan.sample_data = 0;
			return;
		}
	}
	assert(chan.sample_data != 0);
	chan.sample_cursor = mix_samples(target, samples, chan.sample_data, chan.volume, chan.sample_cursor, chan.sample_cursor_delta);
}

void mixer::reset()
{
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		channels[c].sample_data = 0;
		channels[c].sample_cursor = 0;
		channels[c].sample_cursor_delta = 0;
	}
}

s32 sound_mix_buffer[SOUND_BUFFER_SIZE] IWRAM_DATA;

void mixer::mix(s8 *target, size_t samples)
{
	assert(samples > 0);
	
	// zero out the sample-buffer
	u32 zero = 0;
	CpuFastSet(&zero, sound_mix_buffer, DMA_SRC_FIXED | (samples));

	dc_offs = 0;
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		channel_t &chan = (channel_t &)channels[c];
		if (0 != chan.sample_data) mix_channel(chan, sound_mix_buffer, samples);
	}
	dc_offs >>= 8;

	clip_samples(target, sound_mix_buffer, samples, dc_offs);
}
