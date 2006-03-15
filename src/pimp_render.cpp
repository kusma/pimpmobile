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
#include "pimp_render.h"
#include "mixer.h"
#include "math.h"
#include "debug.h"

// #define PRINT_PATTERNS


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

static void set_bpm(pimp_mod_context *ctx, int bpm)
{
	assert(ctx != NULL);
	assert(bpm > 0);
	
	/* we're using 8 fractional-bits for the tick-length */
	ctx->tick_len = int((SAMPLERATE * 5) * (1 << 8)) / (bpm * 2);
}

void init_pimp_mod_context(pimp_mod_context *ctx, const pimp_module_t *mod, const u8 *sample_bank)
{
	assert(ctx != NULL);
	
	ctx->mod = mod;
	ctx->sample_bank = sample_bank;
	
	/* setup default player-state */
	ctx->tick_len = 0;
	ctx->curr_tick_len = 0;
	ctx->curr_row = 0;
	ctx->curr_order = 0;
	ctx->curr_bpm = 125;
	ctx->curr_tempo = 5;
	ctx->curr_tick = 0;
	ctx->global_volume = 1 << 10; /* 24.8 fixed point */
	ctx->curr_pattern = 0;
	
	ctx->curr_pattern = get_pattern(mod, get_order(mod, ctx->curr_order));
	set_bpm(ctx, ctx->mod->bpm);
	ctx->curr_tempo = mod->tempo;
	
#if 0
	iprintf("\n\nperiod range: %d - %d\n", mod->period_low_clamp, mod->period_high_clamp);
	iprintf("orders: %d\nrepeat position: %d\n", mod->order_count, mod->order_repeat);
	iprintf("global volume: %d\ntempo: %d\nbpm: %d\n", mod->volume, mod->tempo, mod->bpm);
	iprintf("channels: %d\n", mod->channel_count);

	for (unsigned i = 0; i < mod->order_count; ++i)
	{
		iprintf("%02x, ", get_order(mod, i));
	}
	
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

	/* todo: move ? */
	mixer::reset();
}

#ifdef PRINT_PATTERNS
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
//	iprintf("%02X ", pe.volume_command);
	iprintf("%02X ", pe.effect_byte);
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
#endif

static pimp_callback callback = 0;
extern "C" void pimp_set_callback(pimp_callback in_callback)
{
	callback = in_callback;
}


void porta_up(pimp_channel_state_t &chan, s32 period_low_clamp)
{
	chan.final_period -= chan.porta_speed;
	if (chan.final_period < period_low_clamp) chan.final_period = period_low_clamp;
}

void porta_down(pimp_channel_state_t &chan, s32 period_high_clamp)
{
	chan.final_period += chan.porta_speed;
	if (chan.final_period > period_high_clamp) chan.final_period = period_high_clamp;
}

void porta_note(pimp_channel_state_t &chan)
{
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
}

void update_row(pimp_mod_context &ctx)
{
	assert(mod != 0);
	
	for (u32 c = 0; c < ctx.mod->channel_count; ++c)
	{
		pimp_channel_state_t      &chan = ctx.channels[c];
		volatile mixer::channel_t &mc   = mixer::channels[c];
		
		const pimp_pattern_entry_t *note = &get_pattern_data(ctx.mod, ctx.curr_pattern)[ctx.curr_row * ctx.mod->channel_count + c];
		
#ifdef PRINT_PATTERNS	
		print_pattern_entry(*note);
#endif
		
		chan.effect           = note->effect_byte;
		chan.effect_param     = note->effect_parameter;
		
		bool period_dirty = false;
		bool volume_dirty = false;
		
		if (note->instrument > 0)
		{
			chan.instrument = get_instrument(ctx.mod, note->instrument - 1);
			chan.sample = get_sample(ctx.mod, chan.instrument, chan.instrument->sample_map[note->note]);
			
			chan.volume = chan.sample->volume;
			volume_dirty = true;
		}
		
		if (chan.instrument != 0 && chan.instrument->sample_count > 0 && note->note > 0 && chan.effect != EFF_PORTA_NOTE && !(chan.effect == EFF_MULTI_FX && chan.effect_param ==  EFF_NOTE_DELAY))
		{
			chan.sample = get_sample(ctx.mod, chan.instrument, chan.instrument->sample_map[note->note]);
			mc.sample_cursor = 0;
			mc.sample_data = ctx.sample_bank + chan.sample->data_ptr;
			mc.sample_length = chan.sample->length;
			mc.loop_type = (mixer::loop_type_t)chan.sample->loop_type;
			mc.loop_start = chan.sample->loop_start;
			mc.loop_end = chan.sample->loop_start + chan.sample->loop_length;
			
			if (ctx.mod->flags & FLAG_LINEAR_PERIODS)
			{
				chan.period = get_linear_period(((s32)note->note) + chan.sample->rel_note, chan.sample->fine_tune);
			}
			else
			{
				chan.period = get_amiga_period(((s32)note->note) + chan.sample->rel_note, chan.sample->fine_tune);
			}
			chan.final_period = chan.period;
			period_dirty = true;
		}
		
		if (note->instrument > 0)
		{
			chan.volume = chan.sample->volume;
			volume_dirty = true;
		}
		
		switch (note->volume_command >> 4)
		{
			case 0x0: break;
			case 0x1:
			case 0x2:
			case 0x3:
			case 0x4:
			case 0x5:
				if (note->volume_command > 0x50)
				{
					/* something else */
					iprintf("unsupported volume-command %02X\n", note->volume_command);
				}
				else
				{
					chan.volume = note->volume_command - 0x10;
					volume_dirty = true;
				}
			break;
			
			default:
				iprintf("unsupported volume-command %02X\n", note->volume_command);
		}
		
		switch (chan.effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				if (chan.effect_param != 0) chan.porta_speed = chan.effect_param * 4;
			break;
			
			case EFF_PORTA_DOWN:
				if (chan.effect_param != 0) chan.porta_speed = chan.effect_param * 4;
			break;
			
			case EFF_PORTA_NOTE:
				if (note->note > 0)
				{
					// no fine tune or relative note here, boooy
					if (ctx.mod->flags & FLAG_LINEAR_PERIODS) chan.porta_target = get_linear_period(note->note + chan.sample->rel_note, 0);
					else chan.porta_target = get_amiga_period(note->note, 0);
					
					/* clamp porta-target period (should not be done for S3M) */
					if (chan.porta_target > ctx.mod->period_high_clamp) chan.porta_target = ctx.mod->period_high_clamp;
					if (chan.porta_target < ctx.mod->period_low_clamp)  chan.porta_target = ctx.mod->period_low_clamp;
				}
				if (chan.effect_param != 0) chan.porta_speed = chan.effect_param * 4;
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
					mc.sample_cursor = (chan.effect_param * 256) << 12;
/*
					if (mc.sample_cursor > mc.sample_length)
					{
						if (mod->flags & FLAG_SAMPLE_OFFSET_CLAMP) mc.sample_cursor = 0; //mc.sample_length;
//						else mc.sample_data = NULL; // kill sample
					}
*/
				}
			break;
			
			case EFF_VOLUME_SLIDE:
				if ((chan.effect_param & 0xF) && (chan.effect_param & 0xF0)) break;
				if (chan.effect_param & 0xF0)
				{
					chan.volume_slide_speed = chan.effect_param >> 4;
				}
				else if (chan.effect_param & 0x0F)
				{
					chan.volume_slide_speed = -(chan.effect_param & 0xF);
				}
			break;
			
/*			case EFF_JUMP_ORDER: break; */

			case EFF_SET_VOLUME:
				chan.volume = chan.effect_param;
				if (chan.volume > 64) chan.volume = 64;
				volume_dirty = true;
			break;

			case EFF_BREAK_ROW:
				ctx.curr_order++;
				ctx.curr_row = (chan.effect_param >> 4) * 10 + (chan.effect_param & 0xF) - 1;
				ctx.curr_pattern = get_pattern(ctx.mod, get_order(ctx.mod, ctx.curr_order));
			break;
			
			case EFF_MULTI_FX:
				switch (chan.effect_param >> 4)
				{
					case EFF_AMIGA_FILTER: break;

					case EFF_FINE_PORTA_UP:
						porta_up(chan, ctx.mod->period_low_clamp);
						period_dirty = true;
					break;
					
					case EFF_FINE_PORTA_DOWN:
						porta_down(chan, ctx.mod->period_high_clamp);
						period_dirty = true;
					break;
					
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
					
					case EFF_NOTE_DELAY:
						chan.note_delay = chan.effect_param & 0xF;
						chan.note = note->note;
					break;
					
					default:
						iprintf("unsupported effect E%X\n", chan.effect_param >> 4);
				}
			break;
			
			case EFF_TEMPO:
				if (note->effect_parameter < 0x20) ctx.curr_tempo = chan.effect_param;
				else set_bpm(&ctx, chan.effect_param);
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
				iprintf("unsupported effect %02X\n", chan.effect);
				assert(0);
		}
		
		if (period_dirty)
		{
			if (ctx.mod->flags & FLAG_LINEAR_PERIODS)
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
			mc.volume = (chan.volume * ctx.global_volume) >> 8;
		}
	}

#ifdef PRINT_PATTERNS	
	iprintf("\n");
#endif

	ctx.curr_tick = 0;
	ctx.curr_row++;
	if (ctx.curr_row == ctx.curr_pattern->row_count)
	{
		ctx.curr_row = 0;
		ctx.curr_order++;
		if (ctx.curr_order >= ctx.mod->order_count) ctx.curr_order = ctx.mod->order_repeat;
		ctx.curr_pattern = get_pattern(ctx.mod, get_order(ctx.mod, ctx.curr_order));
	}
}

static void update_tick(pimp_mod_context &ctx)
{
	if (ctx.mod == 0) return; // no module active (sound-effects can still be playing, though)

	if (ctx.curr_tick == ctx.curr_tempo)
	{
		update_row(ctx);
		ctx.curr_tick++;
		return;
	}
	
	for (u32 c = 0; c < ctx.mod->channel_count; ++c)
	{
		pimp_channel_state_t &chan = ctx.channels[c];
		volatile mixer::channel_t &mc   = mixer::channels[c];
		
		bool period_dirty = false;
		bool volume_dirty = false;
		
		switch (chan.effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				porta_up(chan, ctx.mod->period_low_clamp);
				period_dirty = true;
			break;
			
			case EFF_PORTA_DOWN:
				porta_down(chan, ctx.mod->period_high_clamp);
				period_dirty = true;
			break;
			
			case EFF_PORTA_NOTE:
				porta_note(chan);
				period_dirty = true;
			break;
			
			case EFF_VIBRATO:
			break;
			
			case EFF_VOLUME_SLIDE:
				chan.volume += chan.volume_slide_speed;
				if (chan.volume > 64) chan.volume = 64;
				if (chan.volume < 0) chan.volume = 0;
				volume_dirty = true;
			break;
			
			case EFF_MULTI_FX:
				switch (chan.effect_param >> 4)
				{
					case EFF_AMIGA_FILTER: break;
					case EFF_FINE_VOLUME_SLIDE_UP:
					case EFF_FINE_VOLUME_SLIDE_DOWN:
					break; /* fine volume slide is only done on tick0 */
					
					case EFF_NOTE_DELAY:
						// note on
						if (--chan.note_delay == 0)
						{
							// TODO: replace with a note_on-function
							if (chan.instrument != 0)
							{
								chan.sample = get_sample(ctx.mod, chan.instrument, chan.instrument->sample_map[chan.note]);
								mc.sample_cursor = 0;
								mc.sample_data = ctx.sample_bank + chan.sample->data_ptr;
								mc.sample_length = chan.sample->length;
								mc.loop_type = (mixer::loop_type_t)chan.sample->loop_type;
								mc.loop_start = chan.sample->loop_start;
								mc.loop_end = chan.sample->loop_start + chan.sample->loop_length;
								
								if (ctx.mod->flags & FLAG_LINEAR_PERIODS)
								{
									chan.period = get_linear_period(((s32)chan.note) + chan.sample->rel_note, chan.sample->fine_tune);
								}
								else
								{
									chan.period = get_amiga_period(((s32)chan.note) + chan.sample->rel_note, chan.sample->fine_tune);
								}
								chan.final_period = chan.period;
								period_dirty = true;
							}
						}
					break;

//					default:
//						iprintf("eek %x!\n", chan.effect_param >> 4);
				}
			break;
			
//				default: assert(0);
		}
		
		// period to delta-conversion
		if (period_dirty)
		{
			if (ctx.mod->flags & FLAG_LINEAR_PERIODS)
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
			mc.volume = (chan.volume * ctx.global_volume) >> 8;
		}
	}
	ctx.curr_tick++;
}

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

void pimp_render(pimp_mod_context &ctx, s8 *buf, u32 samples)
{
	static int remainder = 0;
	while (true)
	{
		int samples_to_mix = MIN(remainder, samples);
		if (samples_to_mix != 0) mixer::mix(buf, samples_to_mix);
		
		buf += samples_to_mix;
		samples -= samples_to_mix;
		remainder -= samples_to_mix;
		
		if (samples == 0) break;
		
		PROFILE_COLOR(31, 31, 31);
		update_tick(ctx);
		PROFILE_COLOR(31, 0, 0);
		
		// fixed point tick length
		ctx.curr_tick_len += ctx.tick_len;
		remainder = ctx.curr_tick_len >> 8;
		ctx.curr_tick_len -= (ctx.curr_tick_len >> 8) << 8;
	}
}
