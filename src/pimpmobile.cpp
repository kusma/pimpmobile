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
#include "debug.h"

s8 sound_buffers[2][SOUND_BUFFER_SIZE] IWRAM_DATA;
static u32 sound_buffer_index = 0;

static pimp_channel_state_t channels[CHANNELS];

u32 tick_len = (SOUND_BUFFER_SIZE << 8);
u32 curr_tick_len = 0;

unsigned char *sample_bank;
pimp_module_t *mod;

int get_order(pimp_module_t *mod, int i)
{
	return ((char*)mod + mod->order_ptr)[i];
}

pimp_pattern_t &get_pattern(pimp_module_t *mod, int i)
{
	return ((pimp_pattern_t*)((char*)mod + mod->pattern_ptr))[i];
}

pimp_pattern_entry_t *get_pattern_data(pimp_module_t *mod, pimp_pattern_t &pat)
{
	return (pimp_pattern_entry_t*)((char*)mod + pat.data_ptr);
}

pimp_channel_t &get_channel(pimp_module_t *mod, int i)
{
	return ((pimp_channel_t*)((char*)mod + mod->channel_ptr))[i];
}

void print_pattern(pimp_module_t *mod, pimp_pattern_t &pat)
{
	pimp_pattern_entry_t *pd = get_pattern_data(mod, pat);

	iprintf("%02x\n", pat.row_count);
	
	for (unsigned i = 0; i < 5; ++i)
	{
		for (unsigned j = 0; j < mod->channel_count; ++j)
		{
			pimp_pattern_entry_t &pe = pd[i * mod->channel_count + j];
			
			if (pe.note != 0)
			{
				const int o = (pe.note - 1) / 12;
				const int n = (pe.note - 1) % 12;
				/* C, C#, D, D#, E, F, F#, G, G, A, A#, B */
				printf("%c%c%X ",
					"CCDDEFFGGAAB"[n],
					"-#-#--#-#-#-"[n], o);
			}
			else printf("--- ");
			
//			printf("%02X %02X %X%02X\t", pe.instrument, pe.volume_command, pe.effect_byte, pe.effect_parameter);
		}
		printf("\n");
	}
}

extern "C" void pimp_init(void *module, void *sample_bank)
{
	mod = (pimp_module_t*)module;
	
/*	iprintf("\n\nperiod range: %d - %d\n", mod->period_low_clamp, mod->period_high_clamp);
	iprintf("orders: %d\nrepeat position: %d\n", mod->order_count, mod->order_repeat);
	iprintf("global volume: %d\ntempo: %d\nbpm: %d\n", mod->volume, mod->tempo, mod->bpm); */
	iprintf("channels: %d\n", mod->channel_count);
	iprintf("order ptr: %d\n", mod->order_ptr);
/*
	for (unsigned i = 0; i < mod->order_count; ++i)
	{
		iprintf("%02x, ", get_order(mod, i));
	}
*/
	
	iprintf("pattern ptr: %d\n", mod->pattern_ptr);
	for (unsigned i = 0; i < 1; ++i)
	{
		print_pattern(mod, get_pattern(mod, i));
	}

	for (unsigned i = 0; i < mod->channel_count; ++i)
	{
		pimp_channel_t &c = get_channel(mod, i);
		iprintf("%d %d %d\n", c.volume, c.pan, c.mute);
	}


	if (mod->flags & FLAG_LINEAR_PERIODS) iprintf("LINEAR\n");
	
	
	u32 zero = 0;
	CpuFastSet(&zero, &sound_buffers[0][0], DMA_SRC_FIXED | ((SOUND_BUFFER_SIZE / 4) * 2));
	REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
	REG_SOUNDCNT_X = SOUND_ENABLE;
	
	/* setup timer-shit */
	REG_TM0CNT_L = (1 << 16) - ((1 << 24) / SAMPLERATE);
	REG_TM0CNT_H = TIMER_START;
	
	for (u32 c = 0; c < 1; ++c)
	{
		pimp_channel_state_t &chan = channels[c];
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
	- klipping av samples etter loop-end (kan feile med sample offset)
	- ikke glissando i første omgang

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
		pimp_channel_state_t &chan = channels[c];
		
		pimp_pattern_entry_t *note = get_next_pattern_entry(c);
		
//		unsigned instr = mod->curr_pattern[mod->row][c].sample;
		
		if (note->instrument > 0)
		{
//			NOTE ON !
//			unsigned note = mod->pattern_data[offs].note;
//			unsigned effect = channels[i].effect = mod->pattern_data[offs].effect;
//			unsigned effect_parameters = channels[i].effect_parameters = mod->pattern_data[offs].effect_parameters;
		}
		
		switch (chan.effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				chan.porta_speed = note->effect_parameter;
			break;
			
			case EFF_PORTA_DOWN:
				chan.porta_speed = note->effect_parameter;
			break;
			
			default: assert(0);
		}
	}
}

static void update_tick()
{
	if (mod == 0) return; // no module active (sound-effects can still be playing, though)
	
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		pimp_channel_state_t &chan = channels[c];
		volatile mixer::channel_t &mc   = mixer::channels[c];
		
		bool period_dirty = false;
		
		switch (chan.effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				chan.period -= chan.porta_speed;
				if (chan.period < mod->period_low_clamp) chan.period = mod->period_low_clamp;
				period_dirty = true;
			break;
			
			case EFF_PORTA_DOWN:
				chan.period += chan.porta_speed;
				if (chan.period > mod->period_high_clamp) chan.period = mod->period_high_clamp;
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

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

extern "C" void pimp_frame()
{
	DEBUG_COLOR(31, 31, 31);
	
	u32 samples_left = SOUND_BUFFER_SIZE;
	s8 *buf = sound_buffers[sound_buffer_index];
	
	static int remainder = 0;
	while (true)
	{
		int samples_to_mix = MIN(remainder, samples_left);
		
		if (samples_to_mix != 0) mixer::mix(buf, samples_to_mix);
		DEBUG_COLOR(31, 31, 31);
		buf += samples_to_mix;
		
		samples_left -= samples_to_mix;
		remainder -= samples_to_mix;
		
		if (!samples_left) break;
		
		update_tick();
		
		// fixed point tick length
		curr_tick_len += tick_len;
		remainder = curr_tick_len >> 8;
		curr_tick_len -= (curr_tick_len >> 8) << 8;
	}
	DEBUG_COLOR(0, 0, 0);
}
