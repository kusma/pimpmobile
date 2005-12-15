#include <gba_console.h>
#include <gba_dma.h>
#include <gba_timers.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_sound.h>

#ifndef SNDA_L_ENABLE
#define SNDA_VOL_50     (0<<2)
#define SNDA_VOL_100    (1<<2)
#define SNDB_VOL_50     (0<<3)
#define SNDB_VOL_100    (1<<3)
#define SNDA_R_ENABLE   (1<<8)
#define SNDA_L_ENABLE   (1<<9)
#define SNDA_RESET_FIFO (1<<11)
#define SNDB_R_ENABLE   (1<<12)
#define SNDB_L_ENABLE   (1<<13)
#define SNDB_RESET_FIFO (1<<15)
#define SOUND_ENABLE    (1<<7)
#endif

#include <stdio.h>
#include <assert.h>

#include "internal.h"
#include "mixer.h"
#include "math.h"

s8 sound_buffers[2][SOUND_BUFFER_SIZE] IWRAM_DATA ALIGN(4);
static u32 sound_buffer_index = 0;

static pimp_channel_t channels[CHANNELS];
u32 samples_per_tick = SOUND_BUFFER_SIZE;

unsigned char *sample_bank;
pimp_module_t *mod;

extern "C" void pimp_init(void *module, void *sample_bank)
{
	u32 zero = 0;
	CpuFastSet(&zero, &sound_buffers[0][0], DMA_SRC_FIXED | ((SOUND_BUFFER_SIZE / 4) * 2));
	REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
	REG_SOUNDCNT_X = SOUND_ENABLE;
	
	/* setup timer-shit */
	REG_TM0CNT_L = (1 << 16) - ((1 << 24) / SAMPLERATE);
	REG_TM0CNT_H = TIMER_START;
	
	for (u32 c = 0; c < 1; ++c)
	{
		pimp_channel_t   &chan = channels[c];
		volatile mixer::channel_t &mc   = mixer::channels[c];
		
		chan.period      = 1000;
		chan.effect      = EFF_PORTA_UP;
		chan.porta_speed = 2;
	}
}

extern "C" void pimp_close()
{
	REG_SOUNDCNT_X = 0;
}

extern "C" void pimp_vblank()
{
	if (sound_buffer_index == 0)
	{
		REG_DMA1CNT = 0;
		REG_DMA1SAD = (u32) &(sound_buffers[0][0]);
		REG_DMA1DAD = (u32) &REG_FIFO_A;
		REG_DMA1CNT = DMA_DST_FIXED | DMA_REPEAT | DMA32 | DMA_SPECIAL | DMA_ENABLE;
	}
	sound_buffer_index ^= 1;
}

/*
ideer:
	- tick-lengde-tabell i modul-formatet
	- ping-pong looping med flipping     (kan være tricky med sample offset)
	- klipping av samples etter loop-end (kan feile med sample offset)
	- hvis looping skjer i gjeldende tick - evaluer looping for hver eneste sample
	- ikke glissando i første omgang
	- hvis ping-pong looping skjer, ta overflow i forhold til loop-punkt og trekk fra.

normal-looping:		if (sample_cursor >= loop_end) sample_cursor -= loop_len;
pingpong-looping:	if (sample_cursor_delta < 0 && sample_cursor < loop_start) sample_cursor_delta -= sample_cursor_delta;
					else if (sample_cursor >= loop_end) sample_cursor_delta -= sample_cursor_delta;

miksing:
for each chan
	if looping && loop event (sample-stop is loop event)
		event_cursor = event_point;
		do {
			do
			{
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

pimp_pattern_entry_t *get_next_pattern_entry(unsigned chan)
{
	static int row = -1;
	if (chan == 0) row++;
	return 0;
}

void update_row()
{
	assert(mod != 0);
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		pimp_channel_t &chan = channels[c];
		
		pimp_pattern_entry_t *note = get_next_pattern_entry(c);
		
//		unsigned instr = mod->curr_pattern[mod->row][c].sample;
		
		if (0) // note->instr > 0)
		{
//			NOTE ON !
//			unsigned note = mod->pattern_data[offs].note;
//			unsigned effect = channels[i].effect = mod->pattern_data[offs].effect;
//			unsigned effect_parameters = channels[i].effect_parameters = mod->pattern_data[offs].effect_parameters;
		}
		
		switch (chan.effect)
		{
			case 0x00:
				// effect 0 with parameter 0 is no effect at all
				if (chan.effect_param == 0) continue;
				
			default: assert(0);
		}
	}
}

static void update_tick()
{
//	if (mod == 0) return; // no module active (sound-effects can still be playing, though)
	
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		pimp_channel_t   &chan = channels[c];
		volatile mixer::channel_t &mc   = mixer::channels[c];
		
		bool period_dirty = false;
		
		switch (chan.effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				chan.period -= chan.porta_speed;
				if (chan.period < 1) chan.period = 1;
				period_dirty = true;
			break;
			
			case EFF_PORTA_DOWN:
				chan.period += chan.porta_speed;
//				if (period < 70) period = 0;
				period_dirty = true;
			break;
			
			case EFF_PORTA_NOTE:
				period_dirty = true;
			break;
			
			case EFF_VIBRATO:
			break;
			
			default: assert(0);
		}
		
		if (period_dirty)
		{
			// period to delta-conversion
			mc.sample_cursor_delta = get_amiga_delta(chan.period);
		}
	}
}

#define min(x, y) ((x) > (y) ? (y) : (x))

extern "C" void pimp_frame()
{
	BG_COLORS[0] = RGB5(31, 31, 31);
	
	u32 samples_left = SOUND_BUFFER_SIZE;
	s8 *buf = sound_buffers[sound_buffer_index];
	
	static int remainder = 0;
	while(1)
	{
		int samples_to_mix = min(remainder, samples_left);
		
		if (samples_to_mix != 0) mixer::mix(buf, samples_to_mix);
		BG_COLORS[0] = RGB5(31, 31, 31);
		buf += samples_to_mix;
		
		samples_left -= samples_to_mix;
		remainder -= samples_to_mix;
		
		if (!samples_left) break;
		update_tick();
		remainder = samples_per_tick;
	}
	BG_COLORS[0] = RGB5(0, 0, 0);
}
