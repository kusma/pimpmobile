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

#include "mixer.h"

s8 sound_buffers[2][SOUND_BUFFER_SIZE] IWRAM_DATA ALIGN(4);
static u32 sound_buffer_index = 0;

extern "C" void pimp_init()
{
	u32 zero = 0;
	CpuFastSet(&zero, &sound_buffers[0][0], DMA_SRC_FIXED | ((SOUND_BUFFER_SIZE / 4) * 2));
	REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
	REG_SOUNDCNT_X = SOUND_ENABLE;
	
	/* setup timer-shit */
	REG_TM0CNT_L = (1 << 16) - ((1 << 24) / 18157);
	REG_TM0CNT_H = TIMER_START;
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

/*	Timer = 62610 = 65536 - (16777216 /  5734), buf = 96
	Timer = 63940 = 65536 - (16777216 / 10512), buf = 176
	Timer = 64282 = 65536 - (16777216 / 13379), buf = 224
	Timer = 64612 = 65536 - (16777216 / 18157), buf = 304
	Timer = 64738 = 65536 - (16777216 / 21024), buf = 352
	Timer = 64909 = 65536 - (16777216 / 26758), buf = 448
	Timer = 65004 = 65536 - (16777216 / 31536), buf = 528
	Timer = 65073 = 65536 - (16777216 / 36314), buf = 608
	Timer = 65118 = 65536 - (16777216 / 40137), buf = 672
	Timer = 65137 = 65536 - (16777216 / 42048), buf = 704
	Timer = 65154 = 65536 - (16777216 / 43959), buf = 736 */

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

void update_row()
{
#if 0
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		pimp_channel_t &chan = mod->channels[c];
		unsigned instr = mod->curr_pattern[mod->row][c].sample;
		
		if (instr > 0)
		{
//			NOTE ON !
//			unsigned note = mod->pattern_data[offs].note;
//			unsigned effect = channels[i].effect = mod->pattern_data[offs].effect;
//			unsigned effect_parameters = channels[i].effect_parameters = mod->pattern_data[offs].effect_parameters;
		}
		
		switch (chan.effect)
		{
			case
			default: assert(0);
		}
	}
#endif
}

static void update_tick()
{
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		
	}
}

extern "C" void pimp_frame()
{
	update_tick();
	mixer::mix(sound_buffers[sound_buffer_index], SOUND_BUFFER_SIZE);
}
