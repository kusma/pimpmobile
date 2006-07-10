#include <stdio.h>

#include "../include/pimpmobile.h"
#include "pimp_internal.h"
#include "pimp_render.h"
#include "pimp_debug.h"
#include "pimp_mixer.h"
#include "pimp_math.h"

// #define PRINT_PATTERNS

/* need to move these to a separate channel state header (?) */
STATIC void porta_up(pimp_channel_state *chan, s32 period_low_clamp)
{
	chan->final_period -= chan->porta_speed;
	if (chan->final_period < period_low_clamp) chan->final_period = period_low_clamp;
}

STATIC void porta_down(pimp_channel_state *chan, s32 period_high_clamp)
{
	chan->final_period += chan->porta_speed;
	if (chan->final_period > period_high_clamp) chan->final_period = period_high_clamp;
}

STATIC void porta_note(pimp_channel_state *chan)
{
	if (chan->final_period > chan->porta_target)
	{
		chan->final_period -= chan->porta_speed;
		if (chan->final_period < chan->porta_target) chan->final_period = chan->porta_target;
	}
	else if (chan->final_period < chan->porta_target)
	{
		chan->final_period += chan->porta_speed;
		if (chan->final_period > chan->porta_target) chan->final_period = chan->porta_target;
	}
}

STATIC int __pimp_channel_get_volume(pimp_channel_state *chan)
{
	/* FADEOUT:  */

	int volume = chan->volume;
	
	if (chan->vol_env.env != 0)
	{
		volume = (volume * __pimp_envelope_sample(&chan->vol_env)) >> 8;
		__pimp_envelope_advance_tick(&chan->vol_env, chan->sustain);
		
		volume = (volume * chan->fadeout) >> 16;
		chan->fadeout -= chan->instrument->volume_fadeout;
		if (chan->fadeout <= 0)
		{
			// TODO: kill sample
			chan->fadeout = 0;
		}
	}
	else
	{
		if (!chan->sustain) volume = 0;
	}
	
	return volume;
}

STATIC void __pimp_mod_context_update_row(pimp_mod_context *ctx)
{
	ASSERT(mod != 0);
	
	for (u32 c = 0; c < ctx->mod->channel_count; ++c)
	{
		pimp_channel_state *chan = &ctx->channels[c];
		volatile pimp_mixer_channel_state &mc   = ctx->mixer->channels[c];
		
		const pimp_pattern_entry *note = &get_pattern_data(ctx->curr_pattern)[ctx->curr_row * ctx->mod->channel_count + c];
		
#ifdef PRINT_PATTERNS
		print_pattern_entry(*note);
#endif
		
		chan->effect           = note->effect_byte;
		chan->effect_param     = note->effect_parameter;
		chan->volume_command   = note->volume_command;
		
		bool period_dirty = false;
		bool volume_dirty = false;
		
		if (note->note == KEY_OFF)
		{
			chan->sustain = false;
			volume_dirty  = true; // we need to update volume if note off killed note
		}
		else
		{
			if (note->instrument > 0)
			{
				chan->instrument = __pimp_module_get_instrument(ctx->mod, note->instrument - 1);
				chan->sample  = get_sample(chan->instrument, chan->instrument->sample_map[note->note]);
				
				chan->vol_env.env = get_vol_env(chan->instrument);
				__pimp_envelope_reset(&chan->vol_env);
				chan->sustain = true;
				chan->fadeout = 1 << 16;
				
				chan->volume = chan->sample->volume;
				volume_dirty = true;
			}
			
			if (
				chan->instrument != 0 &&
				note->note > 0 &&
				chan->effect != EFF_PORTA_NOTE &&
				chan->effect != EFF_PORTA_NOTE_VOLUME_SLIDE &&
				!(chan->effect == EFF_MULTI_FX && chan->effect_param ==  EFF_NOTE_DELAY))
			{
				if (chan->instrument->sample_count == 0)
				{
					// stupid musician, tried to play an empty instrument...
					mc.sample_data = NULL;
					mc.sample_cursor = 0;
					mc.sample_cursor_delta = 0;
				}
				else
				{
					chan->sample = get_sample(chan->instrument, chan->instrument->sample_map[note->note]);
					mc.sample_cursor = 0;
					mc.sample_data = ctx->sample_bank + chan->sample->data_ptr;
					mc.sample_length = chan->sample->length;
					mc.loop_type = (pimp_mixer_loop_type)chan->sample->loop_type;
					mc.loop_start = chan->sample->loop_start;
					mc.loop_end = chan->sample->loop_start + chan->sample->loop_length;
					
					if (ctx->mod->flags & FLAG_LINEAR_PERIODS)
					{
						chan->period = __pimp_get_linear_period(((s32)note->note) + chan->sample->rel_note, chan->sample->fine_tune);
					}
					else
					{
						chan->period = __pimp_get_amiga_period(((s32)note->note) + chan->sample->rel_note, chan->sample->fine_tune);
					}
					
					chan->final_period = chan->period;
					period_dirty = true;
				}
			}
		}
		
		if (note->instrument > 0)
		{
			chan->volume = chan->sample->volume;
			volume_dirty = true;
		}

		switch (note->volume_command >> 4)
		{
/*
  0       Do nothing
$10-$50   Set volume Value-$10
  :          :        :
  :          :        :
$60-$6f   Volume slide down
$70-$7f   Volume slide up
$80-$8f   Fine volume slide down
$90-$9f   Fine volume slide up
$a0-$af   Set vibrato speed
$b0-$bf   Vibrato
$c0-$cf   Set panning
$d0-$df   Panning slide left
$e0-$ef   Panning slide right
$f0-$ff   Tone porta
*/
			case 0x0:
			break;
			case 0x1:
			case 0x2:
			case 0x3:
			case 0x4:
			case 0x5:
				if (note->volume_command > 0x50)
				{
					/* something else */
					DEBUG_PRINT(("unsupported volume-command %02X\n", note->volume_command));
				}
				else
				{
					chan->volume = note->volume_command - 0x10;
					DEBUG_PRINT(("setting volume to: %02X\n", chan->volume));
					volume_dirty = true;
				}
			break;
			case 0x6: break;
			
			default:
				DEBUG_PRINT(("unsupported volume-command %02X\n", note->volume_command));
		}
		
		switch (chan->effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				if (chan->effect_param != 0) chan->porta_speed = chan->effect_param * 4;
			break;
			
			case EFF_PORTA_DOWN:
				if (chan->effect_param != 0) chan->porta_speed = chan->effect_param * 4;
			break;
			
			case EFF_PORTA_NOTE:
				if (note->note > 0)
				{
					// no fine tune or relative note here, boooy
					if (ctx->mod->flags & FLAG_LINEAR_PERIODS) chan->porta_target = __pimp_get_linear_period(note->note + chan->sample->rel_note, 0);
					else chan->porta_target = __pimp_get_amiga_period(note->note, 0);
					
					/* clamp porta-target period (should not be done for S3M) */
					if (chan->porta_target > ctx->mod->period_high_clamp) chan->porta_target = ctx->mod->period_high_clamp;
					if (chan->porta_target < ctx->mod->period_low_clamp)  chan->porta_target = ctx->mod->period_low_clamp;
				}
				if (chan->effect_param != 0) chan->porta_speed = chan->effect_param * 4;
			break;
/*
			case EFF_VIBRATO: break; */
			
			case EFF_PORTA_NOTE_VOLUME_SLIDE:
				/* TODO: move to a separate function ? */
				if (note->note > 0)
				{
					// no fine tune or relative note here, boooy
					if (ctx->mod->flags & FLAG_LINEAR_PERIODS) chan->porta_target = __pimp_get_linear_period(note->note + chan->sample->rel_note, 0);
					else chan->porta_target = __pimp_get_amiga_period(note->note, 0);
					
					/* clamp porta-target period (should not be done for S3M) */
					if (chan->porta_target > ctx->mod->period_high_clamp) chan->porta_target = ctx->mod->period_high_clamp;
					if (chan->porta_target < ctx->mod->period_low_clamp)  chan->porta_target = ctx->mod->period_low_clamp;
				}
				
				if (chan->effect_param & 0xF0)
				{
					chan->volume_slide_speed = chan->effect_param >> 4;
				}
				else if (chan->effect_param & 0x0F)
				{
					chan->volume_slide_speed = -(chan->effect_param & 0xF);
				}
			break;
/*
			case EFF_VIBRATO_VOLUME_SLIDE: break;
			case EFF_TREMOLO: break;
			case EFF_SET_PAN: break;
*/
			
			case EFF_SAMPLE_OFFSET:
				if (note->note > 0)
				{
					mc.sample_cursor = (chan->effect_param * 256) << 12;
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
				if (chan->effect_param & 0xF0)
				{
					chan->volume_slide_speed = chan->effect_param >> 4;
				}
				else if (chan->effect_param & 0x0F)
				{
					chan->volume_slide_speed = -(chan->effect_param & 0xF);
				}
			break;
			
/*			case EFF_JUMP_ORDER: break; */

			case EFF_SET_VOLUME:
				chan->volume = chan->effect_param;
				if (chan->volume > 64) chan->volume = 64;
				volume_dirty = true;
			break;

			case EFF_BREAK_ROW:
				ctx->curr_order++;
				ctx->curr_row = (chan->effect_param >> 4) * 10 + (chan->effect_param & 0xF) - 1;
				ctx->curr_pattern = __pimp_module_get_pattern(ctx->mod, __pimp_module_get_order(ctx->mod, ctx->curr_order));
			break;
			
			case EFF_MULTI_FX:
				switch (chan->effect_param >> 4)
				{
					case EFF_AMIGA_FILTER: break;

					case EFF_FINE_PORTA_UP:
						porta_up(chan, ctx->mod->period_low_clamp);
						period_dirty = true;
					break;
					
					case EFF_FINE_PORTA_DOWN:
						porta_down(chan, ctx->mod->period_high_clamp);
						period_dirty = true;
					break;
					
					case EFF_RETRIG_NOTE:
						if ((note->effect_parameter & 0xF) != 0) chan->note_retrig = note->effect_parameter & 0xF;
					break;
					
					case EFF_FINE_VOLUME_SLIDE_UP:
						chan->volume += chan->effect_param & 0xF;
						if (chan->volume > 64) chan->volume = 64;
						volume_dirty = true;
					break;
					
					case EFF_FINE_VOLUME_SLIDE_DOWN:
						chan->volume -= chan->effect_param & 0xF;
						if (chan->volume < 0) chan->volume = 0;
						volume_dirty = true;
					break;
					
					case EFF_NOTE_DELAY:
						chan->note_delay = chan->effect_param & 0xF;
						chan->note = note->note;
					break;
					
					default:
						DEBUG_PRINT(("unsupported effect E%X\n", chan->effect_param >> 4));
				}
			break;
			
			case EFF_TEMPO:
				if (note->effect_parameter < 0x20) ctx->curr_tempo = chan->effect_param;
				else __pimp_mod_context_set_bpm(ctx, chan->effect_param);
			break;
			
/*
			case EFF_SET_GLOBAL_VOLUME: break;
			case EFF_GLOBAL_VOLUME_SLIDE: break;
			case EFF_KEY_OFF: break;
			case EFF_SET_ENVELOPE_POSITION: break;
			case EFF_PAN_SLIDE: break;
*/
			case EFF_MULTI_RETRIG:
				if ((note->effect_parameter & 0xF0) != 0) DEBUG_PRINT(("multi retrig x-parameter != 0 not supported\n"));
				if ((note->effect_parameter & 0xF) != 0) chan->note_retrig = note->effect_parameter & 0xF;
			break;
			
/*			case EFF_TREMOR: break; */
			
			case EFF_SYNC_CALLBACK:
				if (ctx->callback != NULL) ctx->callback(0, chan->effect_param);
			break;
			
/*
			case EFF_ARPEGGIO: break;
			case EFF_SET_TEMPO: break;
			case EFF_SET_BPM: break;
*/
			
			default:
				DEBUG_PRINT(("unsupported effect %02X\n", chan->effect));
				ASSERT(0);
		}
		
		if (period_dirty)
		{
			if (ctx->mod->flags & FLAG_LINEAR_PERIODS)
			{
				mc.sample_cursor_delta = __pimp_get_linear_delta(chan->final_period);
			}
			else
			{
				mc.sample_cursor_delta = __pimp_get_amiga_delta(chan->final_period);
			}
		}
		
		if (volume_dirty || chan->vol_env.env != 0)
		{
			mc.volume = (__pimp_channel_get_volume(chan) * ctx->global_volume) >> 8;
		}
	}

#ifdef PRINT_PATTERNS	
	iprintf("\n");
#endif

	ctx->curr_tick = 0;
	ctx->curr_row++;
	if (ctx->curr_row == ctx->curr_pattern->row_count)
	{
		ctx->curr_row = 0;
		ctx->curr_order++;
		if (ctx->curr_order >= ctx->mod->order_count) ctx->curr_order = ctx->mod->order_repeat;
		ctx->curr_pattern = __pimp_module_get_pattern(ctx->mod, __pimp_module_get_order(ctx->mod, ctx->curr_order));
	}
}

STATIC void __pimp_mod_context_update_tick(pimp_mod_context *ctx)
{
	if (ctx->mod == NULL) return; // no module active (sound-effects can still be playing, though)

	if (ctx->curr_tick == ctx->curr_tempo)
	{
		__pimp_mod_context_update_row(ctx);
		ctx->curr_tick++;
		return;
	}
	
	for (u32 c = 0; c < ctx->mod->channel_count; ++c)
	{
		pimp_channel_state *chan = &ctx->channels[c];
		pimp_mixer_channel_state &mc   = ctx->mixer->channels[c];
		
		bool period_dirty = false;
		bool volume_dirty = false;
		
		switch (chan->volume_command >> 4)
		{
/*
  0       Do nothing
$10-$50   Set volume Value-$10
  :          :        :
  :          :        :
$60-$6f   Volume slide down
$70-$7f   Volume slide up
$80-$8f   Fine volume slide down
$90-$9f   Fine volume slide up
$a0-$af   Set vibrato speed
$b0-$bf   Vibrato
$c0-$cf   Set panning
$d0-$df   Panning slide left
$e0-$ef   Panning slide right
$f0-$ff   Tone porta
*/
			case 0x0:
			case 0x1:
			case 0x2:
			case 0x3:
			case 0x4:
			case 0x5: break;
	
			case 0x6:
				chan->volume -= chan->volume_command & 0xF;
				if (chan->volume < 0) chan->volume = 0;
				volume_dirty = true;
			break;
			
			case 0x7:
				chan->volume += chan->volume_command & 0xF;
				if (chan->volume > 64) chan->volume = 64;
				volume_dirty = true;
			break;
			
			default:
				DEBUG_PRINT(("unsupported volume-command %02X\n", chan->volume_command));
		}
		
		switch (chan->effect)
		{
			case EFF_NONE: break;
			
			case EFF_PORTA_UP:
				porta_up(chan, ctx->mod->period_low_clamp);
				period_dirty = true;
			break;
			
			case EFF_PORTA_DOWN:
				porta_down(chan, ctx->mod->period_high_clamp);
				period_dirty = true;
			break;
			
			case EFF_PORTA_NOTE:
				porta_note(chan);
				period_dirty = true;
			break;
			
			case EFF_VIBRATO:
			break;
			
			case EFF_PORTA_NOTE_VOLUME_SLIDE:
				porta_note(chan);
				period_dirty = true;
				
				/* todo: move to a separate function */
				chan->volume += chan->volume_slide_speed;
				if (chan->volume > 64) chan->volume = 64;
				if (chan->volume < 0) chan->volume = 0;
				volume_dirty = true;
			break;
			
			case EFF_VOLUME_SLIDE:
				chan->volume += chan->volume_slide_speed;
				if (chan->volume > 64) chan->volume = 64;
				if (chan->volume < 0) chan->volume = 0;
				volume_dirty = true;
			break;
			
			case EFF_MULTI_FX:
				switch (chan->effect_param >> 4)
				{
					case EFF_AMIGA_FILTER: break;
					
					case EFF_RETRIG_NOTE:
						chan->retrig_tick++;
						if (chan->retrig_tick == chan->note_retrig)
						{
							mc.sample_cursor = 0;
							chan->retrig_tick = 0;
						}
					break;

					case EFF_FINE_VOLUME_SLIDE_UP:
					case EFF_FINE_VOLUME_SLIDE_DOWN:
					break; /* fine volume slide is only done on tick0 */
					
					case EFF_NOTE_DELAY:
						// note on
						if (--chan->note_delay == 0)
						{
							// TODO: replace with a note_on-function
							if (chan->instrument != 0)
							{
								chan->sample = get_sample(chan->instrument, chan->instrument->sample_map[chan->note]);
								mc.sample_cursor = 0;
								mc.sample_data = ctx->sample_bank + chan->sample->data_ptr;
								mc.sample_length = chan->sample->length;
								mc.loop_type = (pimp_mixer_loop_type)chan->sample->loop_type;
								mc.loop_start = chan->sample->loop_start;
								mc.loop_end = chan->sample->loop_start + chan->sample->loop_length;
								
								if (ctx->mod->flags & FLAG_LINEAR_PERIODS)
								{
									chan->period = __pimp_get_linear_period(((s32)chan->note) + chan->sample->rel_note, chan->sample->fine_tune);
								}
								else
								{
									chan->period = __pimp_get_amiga_period(((s32)chan->note) + chan->sample->rel_note, chan->sample->fine_tune);
								}
								chan->final_period = chan->period;
								period_dirty = true;
							}
						}
					break;
				}
			break;
			
			case EFF_MULTI_RETRIG:
				chan->retrig_tick++;
				if (chan->retrig_tick == chan->note_retrig)
				{
					mc.sample_cursor = 0;
					chan->retrig_tick = 0;
				}
			break;
			
//				default: ASSERT(0);
		}
		
		// period to delta-conversion
		if (period_dirty)
		{
			if (ctx->mod->flags & FLAG_LINEAR_PERIODS)
			{
				mc.sample_cursor_delta = __pimp_get_linear_delta(chan->final_period);
			}
			else
			{
				mc.sample_cursor_delta = __pimp_get_amiga_delta(chan->final_period);
			}
		}
		
		if (volume_dirty || chan->vol_env.env != 0)
		{
			mc.volume = (__pimp_channel_get_volume(chan) * ctx->global_volume) >> 8;
		}
	}
	
	ctx->curr_tick++;
}

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

void __pimp_render(pimp_mod_context *ctx, s8 *buf, u32 samples)
{
	static int remainder = 0;
	while (true)
	{
		int samples_to_mix = MIN(remainder, samples);
		if (samples_to_mix != 0) __pimp_mixer_mix(ctx->mixer, buf, samples_to_mix);
		
		buf       += samples_to_mix;
		samples   -= samples_to_mix;
		remainder -= samples_to_mix;
		
		if (samples == 0) break;
		
		__pimp_mod_context_update_tick(ctx);
		
		// fixed point tick length
		ctx->curr_tick_len += ctx->tick_len;
		remainder           = ctx->curr_tick_len >> 8;
		ctx->curr_tick_len -= (ctx->curr_tick_len >> 8) << 8;
	}
}
