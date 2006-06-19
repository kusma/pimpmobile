#include "pimp_mixer.h"
#include "pimp_debug.h"

#include <gba_systemcalls.h>
#include <gba_dma.h>

static s32 event_delta = 0;
static s32 event_cursor = 0;

static BOOL detect_loop_event(pimp_mixer_channel_state *chan, int samples)
{
	ASSERT(samples != 0);
	
	s32 check_point;
	s32 end_sample = chan->sample_cursor + chan->sample_cursor_delta * samples;
	
	switch (chan->loop_type)
	{
		case LOOP_TYPE_NONE:
			check_point = chan->sample_length;
		break;
		
		case LOOP_TYPE_FORWARD:
			check_point = chan->loop_end;
		break;
		
		case LOOP_TYPE_PINGPONG:
			if (chan->sample_cursor_delta >= 0)
			{
				// moving forwards through the sample
				check_point = chan->loop_end;
			}
			else
			{
				// moving backwards through the sample
				check_point = chan->loop_start;
				if (end_sample < (check_point << 12))
				{
					event_delta = -chan->sample_cursor_delta;
					event_cursor = -((check_point << 12) - chan->sample_cursor);
					return TRUE;
				}
				return FALSE;
			}
		break;
	}
	
	if (end_sample >= (check_point << 12))
	{
		event_delta = chan->sample_cursor_delta;
		event_cursor = (check_point << 12) - chan->sample_cursor;
		return TRUE;
	}
	
	return FALSE;
}

// returns false if we hit sample-end
BOOL process_loop_event(pimp_mixer_channel_state *chan)
{
	switch (chan->loop_type)
	{
		case LOOP_TYPE_NONE:
			return FALSE;
		break;
		
		case LOOP_TYPE_FORWARD:
			do
			{
				chan->sample_cursor -= (chan->loop_end - chan->loop_start) << 12;
			}
			while (chan->sample_cursor >= (chan->loop_end << 12));
		break;
		
		case LOOP_TYPE_PINGPONG:
			do
			{
				if (chan->sample_cursor_delta >= 0)
				{
					chan->sample_cursor -=  chan->loop_end << 12;
					chan->sample_cursor  = -chan->sample_cursor;
					chan->sample_cursor +=  chan->loop_end << 12;
				}
				else
				{
					chan->sample_cursor -=  chan->loop_start << 12;
					chan->sample_cursor  = -chan->sample_cursor;
					chan->sample_cursor +=  chan->loop_start << 12;
				}
				chan->sample_cursor_delta = -chan->sample_cursor_delta;
			}
			while ((chan->sample_cursor > (chan->loop_end << 12)) || (chan->sample_cursor < (chan->loop_start << 12)));
		break;
	}

	return TRUE;
}

u32 dc_offs = 0;

void __pimp_mixer_mix_channel(pimp_mixer_channel_state *chan, s32 *target, u32 samples)
{
	if (chan->volume < 1) return;
	dc_offs += chan->volume * 128;
	
	ASSERT(samples > 0);
	
	while (samples > 0 && detect_loop_event(chan, samples) == true)
	{
		do
		{
			ASSERT((chan->sample_cursor >> 12) < chan->sample_length);
			ASSERT(chan->sample_data != 0);
			
			s32 samp = ((u8*)chan->sample_data)[chan->sample_cursor >> 12];
			chan->sample_cursor += chan->sample_cursor_delta;
			*target++ += samp * chan->volume;
			
			samples--;
			event_cursor -= event_delta;
		}
		while (event_cursor > 0);
		
		ASSERT(samples >= 0);
		
		if (process_loop_event(chan) == false)
		{
			// the sample has stopped, we need to fill the rest of the buffer with the dc-offset, so it doesn't ruin our unsigned mixing-thing
			while (samples--)
			{
				*target++ += chan->volume * 128;
			}
			// terminate sample
			chan->sample_data = 0;
			return;
		}
	}
	ASSERT(chan->sample_data != 0);
	chan->sample_cursor = __pimp_mixer_mix_samples(target, samples, chan->sample_data, chan->volume, chan->sample_cursor, chan->sample_cursor_delta);
}

void __pimp_mixer_reset(pimp_mixer *mixer)
{
	ASSERT(mixer != NULL);
	
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		mixer->channels[c].sample_data = 0;
		mixer->channels[c].sample_cursor = 0;
		mixer->channels[c].sample_cursor_delta = 0;
	}
}

static s32 sound_mix_buffer[SOUND_BUFFER_SIZE] IWRAM_DATA;

void __pimp_mixer_mix(pimp_mixer *mixer, s8 *target, int samples)
{
	ASSERT(samples > 0);
	
	// zero out the sample-buffer
	u32 zero = 0;
	CpuFastSet(&zero, sound_mix_buffer, DMA_SRC_FIXED | (samples));

	dc_offs = 0;
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		pimp_mixer_channel_state *chan = &mixer->channels[c];
		if (NULL != chan->sample_data) __pimp_mixer_mix_channel(chan, sound_mix_buffer, samples);
	}
	dc_offs >>= 8;

	__pimp_mixer_clip_samples(target, sound_mix_buffer, samples, dc_offs);
}
