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

#include "../include/pimpmobile.h"
#include "internal.h"
#include "mixer.h"
#include "math.h"
#include "debug.h"

static s8 sound_buffers[2][SOUND_BUFFER_SIZE] IWRAM_DATA;
static u32 sound_buffer_index = 0;

/* should be part of a kind of player-context ? */
static pimp_channel_state_t channels[CHANNELS];
static u32 tick_len = 0;
static u32 curr_tick_len = 0;
static u32 curr_row = 0;
static u32 curr_order = 0;
static u32 curr_bpm = 125;
static u32 curr_tempo = 5;
static u32 curr_tick = 0;
static pimp_pattern_t *curr_pattern = 0;

static const unsigned char *pimp_sample_bank;
static const pimp_module_t *mod;


static void set_bpm(int bpm)
{
	assert(bpm > 0);
	/* the shift is because we're using 8 fractional-bits for the tick-length */
	tick_len = ((SAMPLERATE * 5) << 8) / (bpm * 2);
}

static int get_order(const pimp_module_t *mod, int i)
{
	return ((char*)mod + mod->order_ptr)[i];
}

static pimp_pattern_t *get_pattern(const pimp_module_t *mod, int i)
{
	return &((pimp_pattern_t*)((char*)mod + mod->pattern_ptr))[i];
}

static pimp_pattern_entry_t *get_pattern_data(const pimp_module_t *mod, pimp_pattern_t *pat)
{
	return (pimp_pattern_entry_t*)((char*)mod + pat->data_ptr);
}

static pimp_channel_t &get_channel(const pimp_module_t *mod, int i)
{
	return ((pimp_channel_t*)((char*)mod + mod->channel_ptr))[i];
}

static pimp_instrument_t *get_instrument(const pimp_module_t *mod, int i)
{
	return &((pimp_instrument_t*)((char*)mod + mod->instrument_ptr))[i];
}

static pimp_sample_t *get_sample(const pimp_module_t *mod, pimp_instrument_t *instr, int i)
{
	return &((pimp_sample_t*)((char*)mod + instr->sample_ptr))[i];
}


void print_pattern_entry(const pimp_pattern_entry_t &pe)
{
	if (pe.note != 0)
	{
		const int o = (pe.note - 1) / 12;
		const int n = (pe.note - 1) % 12;
		/* C, C#, D, D#, E, F, F#, G, G, A, A#, B */
		iprintf("%c%c%X ",
			"CCDDEFFGGAAB"[n],
			"-#-#--#-#-#-"[n], o);
	}
	else iprintf("--- ");
//	iprintf("%02X ", pe.instrument);
//	iprintf("%02X %02X %X%02X\t", pe.instrument, pe.volume_command, pe.effect_byte, pe.effect_parameter);
}

void print_pattern(const pimp_module_t *mod, pimp_pattern_t *pat)
{
	pimp_pattern_entry_t *pd = get_pattern_data(mod, pat);

	iprintf("%02x\n", pat->row_count);
	
	for (unsigned i = 0; i < 5; ++i)
	{
		for (unsigned j = 0; j < 4; ++j)
		{
			pimp_pattern_entry_t &pe = pd[i * mod->channel_count + j];
			print_pattern_entry(pe);
		}
		iprintf("\n");
	}
}

static pimp_callback callback = 0;
extern "C" void pimp_set_callback(pimp_callback in_callback)
{
	callback = in_callback;
}

extern "C" void pimp_init(const void *module, const void *sample_bank)
{
	mod = (const pimp_module_t*)module;
	pimp_sample_bank = (const u8*)sample_bank;
	
	mixer::reset();
	
	/* setup default player-state */
	curr_row = 0;
	curr_tick = 0;
	curr_tick_len = 0;

	curr_order = 0;
	curr_pattern = get_pattern(mod, get_order(mod, curr_order));
	
	set_bpm(mod->bpm);
	curr_tempo = mod->tempo;
#if 0
/*	iprintf("\n\nperiod range: %d - %d\n", mod->period_low_clamp, mod->period_high_clamp);
	iprintf("orders: %d\nrepeat position: %d\n", mod->order_count, mod->order_repeat);
	iprintf("global volume: %d\ntempo: %d\nbpm: %d\n", mod->volume, mod->tempo, mod->bpm); */
/*	iprintf("channels: %d\n", mod->channel_count); */

/*
	for (unsigned i = 0; i < mod->order_count; ++i)
	{
		iprintf("%02x, ", get_order(mod, i));
	}
*/
	
	iprintf("pattern ptr: %d\n", mod->pattern_ptr);
	for (unsigned i = 0; i < 1; ++i)
	{
		print_pattern(mod, get_pattern(mod, get_order(mod, i)));
	}

	for (unsigned i = 0; i < mod->channel_count; ++i)
	{
		pimp_channel_t &c = get_channel(mod, i);
		iprintf("%d %d %d\n", c.volume, c.pan, c.mute);
	}

	if (mod->flags & FLAG_LINEAR_PERIODS) iprintf("LINEAR\n");
	
	for (unsigned i = 0; i < mod->instrument_count; ++i)
	{
		pimp_instrument_t *instr = get_instrument(mod, i);
		iprintf("%d ",  instr->sample_count);
		iprintf("%d\n", instr->sample_map[3]);
	}
#endif

	u32 zero = 0;
	CpuFastSet(&zero, &sound_buffers[0][0], DMA_SRC_FIXED | ((SOUND_BUFFER_SIZE / 4) * 2));
	REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
	REG_SOUNDCNT_X = SOUND_ENABLE;
	
	/* setup timer-shit */
	REG_TM0CNT_L = (1 << 16) - ((1 << 24) / SAMPLERATE);
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

void update_row()
{
	assert(mod != 0);
	
	for (u32 c = 0; c < mod->channel_count; ++c)
	{
		pimp_channel_state_t &chan = channels[c];
		volatile mixer::channel_t &mc   = mixer::channels[c];

		const pimp_pattern_entry_t *note = &get_pattern_data(mod, curr_pattern)[curr_row * mod->channel_count + c];
		
		if (c < 5) print_pattern_entry(*note);
		
		chan.effect           = note->effect_byte;
		chan.effect_param     = note->effect_parameter;
		
		bool period_dirty = false;
		bool volume_dirty = false;
		
//		NOTE ON !
		if (note->instrument > 0 && chan.effect != EFF_PORTA_NOTE)
		{
			pimp_instrument_t *instr = get_instrument(mod, note->instrument - 1);
			pimp_sample_t *samp = get_sample(mod, instr, instr->sample_map[note->note]);
			
			int period;
			if (mod->flags & FLAG_LINEAR_PERIODS)
			{
				period = get_linear_period(note->note, samp->fine_tune);
			}
			else
			{
				period = get_amiga_period(note->note, samp->fine_tune);
			}
			
			chan.period = chan.final_period = period;
			period_dirty = true;
			
			mc.sample_cursor = 0;
			mc.sample_data = pimp_sample_bank + samp->data_ptr;
			mc.sample_length = samp->length;
			mc.loop_type = (mixer::loop_type_t)samp->loop_type;
			mc.loop_start = samp->loop_start;
			mc.loop_end = samp->loop_start + samp->loop_length;
		}
		
		if (note->instrument > 0)
		{
			pimp_instrument_t *instr = get_instrument(mod, note->instrument - 1);
			pimp_sample_t *samp = get_sample(mod, instr, instr->sample_map[note->note]);
			chan.volume = samp->volume;
			volume_dirty = true;
		}
		
		switch (chan.effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				chan.porta_speed = chan.effect_param * 4;
			break;
			
			case EFF_PORTA_DOWN:
				chan.porta_speed = chan.effect_param * 4;
			break;
			
			case EFF_PORTA_NOTE:
				if (note->note > 0)
				{
					if (mod->flags & FLAG_LINEAR_PERIODS) chan.porta_target = get_linear_period(note->note, 0);
					else chan.porta_target = get_amiga_period(note->note, 0);
					
					/* clamp porta-target period (should not be done for S3M) */
					if (chan.porta_target > mod->period_high_clamp) chan.porta_target = mod->period_high_clamp;
					if (chan.porta_target < mod->period_low_clamp)  chan.porta_target = mod->period_low_clamp;
				}
				chan.porta_speed = chan.effect_param * 4;
			break;
/*
			case EFF_VIBRATO: break;
			case EFF_PORTA_NOTE_VOLUME_SLIDE: break;
			case EFF_VIBRATO_VOLUME_SLIDE: break;
			case EFF_TREMOLO: break;
			case EFF_SET_PAN: break;
*/
			
			case EFF_SAMPLE_OFFSET:
				if (note->note > 0)
				{
					mixer::channels[c].sample_cursor = (chan.effect_param * 256) << 12;
				}
			break;
			
			case EFF_VOLUME_SLIDE: break;
			
/*			case EFF_JUMP_ORDER: break; */

			case EFF_SET_VOLUME:
				chan.volume = chan.effect_param;
				if (chan.volume > 64) chan.volume = 64;
				volume_dirty = true;
			break;

			case EFF_BREAK_ROW:
				curr_order++;
				curr_row = (chan.effect_param >> 4) * 10 + (chan.effect_param & 0xF) - 1;
				curr_pattern = get_pattern(mod, get_order(mod, curr_order));
			break;
			
			case EFF_MULTI_FX:
				switch (chan.effect_param >> 4)
				{
					case EFF_AMIGA_FILTER: break;
					
					case EFF_FINE_VOLUME_SLIDE_UP:
						chan.volume += chan.effect_param & 0xF;
						if (chan.volume > 64) chan.volume = 64;
						volume_dirty = true;
					break;
					
					case EFF_FINE_VOLUME_SLIDE_DOWN:
						chan.volume -= chan.effect_param & 0xF;
						if (chan.volume < 0) chan.volume = 0;
						volume_dirty = true;
					break;
/*
					case EFF_NOTE_DELAY:
						chan.note = note->note;
					break;
*/
					default:
						iprintf("eek E%X\n", chan.effect_param >> 4);
				}
			break;
			
			case EFF_TEMPO:
				if (note->effect_parameter < 0x20) curr_tempo = chan.effect_param;
				else set_bpm(chan.effect_param);
			break;
			
/*
			case EFF_SET_GLOBAL_VOLUME: break;
			case EFF_GLOBAL_VOLUME_SLIDE: break;
			case EFF_KEY_OFF: break;
			case EFF_SET_ENVELOPE_POSITION: break;
			case EFF_PAN_SLIDE: break;
			case EFF_MULTI_RETRIG: break;
			case EFF_TREMOR: break;
*/
			
			case EFF_SYNC_CALLBACK:
				if (callback != 0) callback(0, chan.effect_param);
			break;
			
/*
			case EFF_ARPEGGIO: break;
			case EFF_SET_TEMPO: break;
			case EFF_SET_BPM: break;
*/
			
			default:
				iprintf("eek %02X!\n", chan.effect);
//				assert(0);
		}
		
		if (period_dirty)
		{
			if (mod->flags & FLAG_LINEAR_PERIODS)
			{
				mc.sample_cursor_delta = get_linear_delta(chan.final_period);
			}
			else
			{
				mc.sample_cursor_delta = get_amiga_delta(chan.final_period);
			}
		}
		
		if (volume_dirty)
		{
			mc.volume = chan.volume * 2;
		}
	}
	
	iprintf("\n");
	
	curr_tick = 0;
	curr_row++;
	if (curr_row == curr_pattern->row_count)
	{
		curr_row = 0;
		curr_order++;
		if (curr_order >= mod->order_count) curr_order = mod->order_repeat;
		curr_pattern = get_pattern(mod, get_order(mod, curr_order));
	}
}

static void update_tick()
{
	if (mod == 0) return; // no module active (sound-effects can still be playing, though)

	if (curr_tick == curr_tempo)
	{
		update_row();
		curr_tick++;
		return;
	}
	
	for (u32 c = 0; c < mod->channel_count; ++c)
	{
		pimp_channel_state_t &chan = channels[c];
		volatile mixer::channel_t &mc   = mixer::channels[c];
		
		bool period_dirty = false;
		bool volume_dirty = false;
		
		switch (chan.effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				chan.final_period -= chan.porta_speed;
				if (chan.final_period > mod->period_high_clamp) chan.final_period = mod->period_high_clamp;
				if (chan.final_period < mod->period_low_clamp) chan.final_period = mod->period_low_clamp;
				period_dirty = true;
			break;
			
			case EFF_PORTA_DOWN:
				chan.final_period += chan.porta_speed;
				if (chan.final_period > mod->period_high_clamp) chan.final_period = mod->period_high_clamp;
				if (chan.final_period < mod->period_low_clamp) chan.final_period = mod->period_low_clamp;
				period_dirty = true;
			break;
			
			case EFF_PORTA_NOTE:
				if (chan.final_period > chan.porta_target)
				{
					chan.final_period -= chan.porta_speed;
					if (chan.final_period < chan.porta_target) chan.final_period = chan.porta_target;
				}
				else if (chan.final_period < chan.porta_target)
				{
					chan.final_period += chan.porta_speed;
					if (chan.final_period > chan.porta_target) chan.final_period = chan.porta_target;
				}
				period_dirty = true;
			break;
			
			case EFF_VOLUME_SLIDE:
				// should be removed in exporter
				if ((chan.effect_param & 0xF) && (chan.effect_param & 0xF0)) break;
				if (chan.effect_param & 0xF0)
				{
					chan.volume += chan.effect_param >> 4;
					if (chan.volume > 64) chan.volume = 64;
					volume_dirty = true;
				}
				else
				{
					chan.volume -= (chan.effect_param & 0xF) * 4;
					if (chan.volume < 0) chan.volume = 0;
					volume_dirty = true;
				}
			break;
			
			case EFF_MULTI_FX:
				switch (chan.effect_param >> 4)
				{
					case EFF_AMIGA_FILTER: break;
					case EFF_FINE_VOLUME_SLIDE_UP:
					case EFF_FINE_VOLUME_SLIDE_DOWN:
					break; /* fine volume slide is only done on tick0 */
					
//					default:
//						iprintf("eek %x!\n", chan.effect_param >> 4);
				}
			break;
			
			case EFF_VIBRATO:
			break;
			
//				default: assert(0);
		}
		
		// period to delta-conversion
		if (period_dirty)
		{
			if (mod->flags & FLAG_LINEAR_PERIODS)
			{
				mc.sample_cursor_delta = get_linear_delta(chan.final_period);
			}
			else
			{
				mc.sample_cursor_delta = get_amiga_delta(chan.final_period);
			}
		}
		
		if (volume_dirty)
		{
			mc.volume = chan.volume * 2;
		}
	}
	curr_tick++;
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
		
		DEBUG_COLOR(0, 0, 0);
		update_tick();
		DEBUG_COLOR(31, 31, 31);
//		printf("%d %d %d %d\n", mod->bpm, curr_row, curr_order, curr_tick);
		
		// fixed point tick length
		curr_tick_len += tick_len;
		remainder = curr_tick_len >> 8;
		curr_tick_len -= (curr_tick_len >> 8) << 8;
	}
	DEBUG_COLOR(0, 0, 0);
}
