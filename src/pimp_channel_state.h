/* pimp_channel_state.h -- The state for each module-channel
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_CHANNEL_STATE_H
#define PIMP_CHANNEL_STATE_H

#include "pimp_types.h"
#include "pimp_envelope.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pimp_instrument;
struct pimp_sample;

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

#ifdef __cplusplus
}
#endif

#endif /* PIMP_CHANNEL_STATE_H */
