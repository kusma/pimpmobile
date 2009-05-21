/* pimp_mod_context.c -- The rendering-context for a module
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include "pimp_mod_context.h"

void pimp_mod_context_init(struct pimp_mod_context *ctx, const pimp_module *mod, const u8 *sample_bank, struct pimp_mixer *mixer, const float samplerate)
{
	int i;
	ASSERT(ctx != NULL);
	
	ctx->mod = mod;
	ctx->sample_bank = sample_bank;
	ctx->mixer = mixer;
	pimp_mod_context_set_samplerate(ctx, samplerate);
	
	/* setup default player-state */
	ctx->tick_len      = 0;
	ctx->curr_tick_len = 0;
	
	ctx->curr_row      = 0;
	ctx->curr_order    = 0;
	ctx->curr_pattern  = (struct pimp_pattern*)NULL;
	ctx->curr_tick     = 0;
	
	ctx->next_row      = 0;
	ctx->next_order    = 0;
	ctx->next_pattern = pimp_module_get_pattern(ctx->mod, pimp_module_get_order(ctx->mod, ctx->next_order));
	
	ctx->curr_bpm   = 125;
	ctx->curr_tempo = 5;
	ctx->remainder  = 0;
	
	ctx->global_volume = 1 << 9; /* 24.8 fixed point */
	
	ctx->curr_pattern = pimp_module_get_pattern(mod, pimp_module_get_order(mod, ctx->curr_order));
	pimp_mod_context_set_bpm(ctx, ctx->mod->bpm);
	ctx->curr_tempo = mod->tempo;
	ctx->curr_tick  = ctx->curr_tempo; /* make sure we skip to the next row right away */
	
	for (i = 0; i < PIMP_CHANNEL_COUNT; ++i)
	{
		struct pimp_channel_state *chan = &ctx->channels[i];
		chan->instrument  = (const struct pimp_instrument*)NULL;
		chan->sample      = (const struct pimp_sample*)    NULL;
		chan->vol_env.env = (const struct pimp_envelope*)  NULL;
		
		chan->volume_command = 0;
		chan->effect = 0;
		chan->effect_param = 0;
		
		chan->note_delay = 0;
		chan->note_retrig = 0;
		chan->retrig_tick = 0;
		chan->porta_target = 0;
		chan->porta_speed = 0;
		chan->fadeout = 0;
		chan->volume_slide_speed = 0;
		
		chan->loop_target_order = 0;
		chan->loop_target_row   = 0;
		chan->loop_counter      = 0;
		
		pimp_envelope_reset(&chan->vol_env);
	}
	
	ctx->callback = (pimp_callback)NULL;
	
	pimp_mixer_reset(ctx->mixer);
}

void pimp_mod_context_set_samplerate(struct pimp_mod_context *ctx, const float samplerate)
{
	ctx->samplerate = samplerate;
	ctx->delta_scale = (unsigned int)((1.0 / samplerate) * (1 << 6) * (1ULL << 32));
}

/* "hard" jump in a module */
void pimp_mod_context_set_pos(struct pimp_mod_context *ctx, int row, int order)
{
	ASSERT(ctx != NULL);
	
	if (1)
	{
		/* skip ahead to next pos right away */
		ctx->curr_tick = ctx->curr_tempo;
		ctx->remainder  = 0;
	}
	
	ctx->next_row = MAX(row, 0);
	ctx->next_order = MAX(order, 0);
	if (ctx->next_order >= ctx->mod->order_count) ctx->next_order = ctx->mod->order_repeat;
	ctx->next_pattern = pimp_module_get_pattern(ctx->mod, pimp_module_get_order(ctx->mod, ctx->next_order));
	if (ctx->next_row >= ctx->next_pattern->row_count) ctx->next_row = ctx->next_pattern->row_count - 1;
	
#if 0
	/* wrap row (seek forward) */
	while (ctx->next_row >= ctx->next_pattern->row_count)
	{
		ctx->next_row -= ctx->next_pattern->row_count;
		ctx->next_order++;
		if (ctx->next_order >= ctx->mod->order_count)
		{
			ctx->next_order = ctx->mod->order_repeat;
		}
		ctx->next_pattern = pimp_module_get_pattern(ctx->mod, pimp_module_get_order(ctx->mod, ctx->next_order));
	}
#endif
}

/* make sure next pos isn't outside a pattern or the order-list */
static void pimp_mod_context_fix_next_pos(struct pimp_mod_context *ctx)
{
	if (ctx->next_row == ctx->curr_pattern->row_count)
	{
		ctx->next_row = 0;
		ctx->next_order++;
		
		/* check for pattern loop */
		if (ctx->next_order >= ctx->mod->order_count) ctx->next_order = ctx->mod->order_repeat;
	}
	
	if (ctx->next_order != ctx->curr_order)
	{
		ctx->next_pattern = pimp_module_get_pattern(ctx->mod, pimp_module_get_order(ctx->mod, ctx->next_order));
	}
}

/* setup next position to be one row advanced in module */
void pimp_mod_context_update_next_pos(struct pimp_mod_context *ctx)
{
	ctx->next_row = ctx->curr_row + 1;
	ctx->next_order = ctx->curr_order;
	pimp_mod_context_fix_next_pos(ctx);
}

/* setup next position to be a specific position. useful for jumping etc */
void pimp_mod_context_set_next_pos(struct pimp_mod_context *ctx, int row, int order)
{
	ASSERT(ctx != NULL);
	
	ctx->next_row = row;
	ctx->next_order = order;
	pimp_mod_context_fix_next_pos(ctx);
}


void pimp_mod_context_set_bpm(struct pimp_mod_context *ctx, int bpm)
{
	/* we're using 8 fractional-bits for the tick-length */
	const int temp = (int)((ctx->samplerate * 5) * (1 << 8));
	
	ASSERT(ctx != NULL);
	ASSERT(bpm > 0);
	
	ctx->tick_len = temp / (bpm * 2);
}
