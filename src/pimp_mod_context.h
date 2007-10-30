/* pimp_mod_context.h -- The rendering-context for a module
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_MOD_CONTEXT_H
#define PIMP_MOD_CONTEXT_H

#include "../include/pimp_gba.h" /* needed for pimp_callback */
#include "pimp_module.h"
#include "pimp_mixer.h"

#include "pimp_instrument.h"
#include "pimp_envelope.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pimp_channel_state
{
	/* some current-states */
	const struct pimp_instrument *instrument;
	const struct pimp_sample     *sample;
	
	struct pimp_envelope_state vol_env;
	BOOL sustain;
	
	u8  note;
	u8  effect;
	u8  effect_param;
	u8  volume_command;

	s32 period;
	s32 final_period;
	
	s8  volume;
	u8  pan;
	
	/* vibrato states */
	u8 vibrato_speed;
	u8 vibrato_depth;
	u8 vibrato_waveform;
	u8 vibrato_counter;
	
	/* pattern loop states */
	u8 loop_target_order;
	u8 loop_target_row;
	u8 loop_counter;
		
	s32 porta_target;
	s32 fadeout;
	u16 porta_speed;
	s8  volume_slide_speed;
	u8  note_delay;
		
	u8  note_retrig;
	u8  retrig_tick;
};

struct pimp_mod_context
{
	u32 tick_len;
	u32 curr_tick_len;
	u32 remainder;

	s32 curr_row;
	s32 curr_order;
	
	u32 next_row;
	u32 next_order;
	
	/* used to delay row / order getters. usefull for demo-synching */
	u32 report_row;
	u32 report_order;
	
	u32 curr_bpm;
	u32 curr_tempo;
	u32 curr_tick;
	s32 global_volume; /* 24.8 fixed point */

	struct pimp_pattern *curr_pattern;
	struct pimp_pattern *next_pattern;
	
	struct pimp_channel_state channels[CHANNELS];
	
	const u8          *sample_bank;
	const pimp_module *mod;
	pimp_mixer        *mixer;
	
	pimp_callback callback;
};

void pimp_mod_context_init(struct pimp_mod_context *ctx, const pimp_module *mod, const u8 *sample_bank, pimp_mixer *mixer);
void pimp_mod_context_set_bpm(struct pimp_mod_context *ctx, int bpm);
void pimp_mod_context_set_tempo(struct pimp_mod_context *ctx, int tempo);

/* position manipulation */
void pimp_mod_context_set_pos(struct pimp_mod_context *ctx, int row, int order);
void pimp_mod_context_set_next_pos(struct pimp_mod_context *ctx, int row, int order);
void pimp_mod_context_update_next_pos(struct pimp_mod_context *ctx);

static INLINE int pimp_mod_context_get_row(const struct pimp_mod_context *ctx)
{
	return ctx->report_row;
}

static INLINE int pimp_mod_context_get_order(const struct pimp_mod_context *ctx)
{
	return ctx->report_order;
}

static INLINE int pimp_mod_context_get_bpm(const struct pimp_mod_context *ctx)
{
	return ctx->curr_bpm;
}

static INLINE int pimp_mod_context_get_tempo(const struct pimp_mod_context *ctx)
{
	return ctx->curr_tempo;
}

#ifdef __cplusplus
}
#endif

#endif /* PIMP_MOD_CONTEXT_H */
