/* pimp_mixer.h -- The state and interface for the various mixers
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_MIXER_H
#define PIMP_MIXER_H

#include "pimp_base.h"
#include "pimp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

enum pimp_mixer_loop_type
{
	LOOP_TYPE_NONE,
	LOOP_TYPE_FORWARD,
	LOOP_TYPE_PINGPONG
};

struct pimp_mixer_channel_state
{
	u32                        sample_length;
	u32                        loop_start;
	u32                        loop_end;
	enum pimp_mixer_loop_type  loop_type;
	const u8                  *sample_data;
	u32                        sample_cursor;
	s32                        sample_cursor_delta;
	s32                        volume;
};

int pimp_mixer_detect_loop_event(const struct pimp_mixer_channel_state *chan, int samples);

struct pimp_mixer
{
	struct pimp_mixer_channel_state channels[CHANNELS];
	s32 *mix_buffer;
};

void pimp_mixer_reset(struct pimp_mixer *mixer);
void pimp_mixer_mix(struct pimp_mixer *mixer, s8 *target, int samples);
void pimp_mixer_mix_channel(struct pimp_mixer_channel_state *chan, s32 *target, u32 samples);

void pimp_mixer_clear(s32 *target, u32 samples);
u32  pimp_mixer_mix_samples(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta);
void pimp_mixer_clip_samples(s8 *target, const s32 *source, u32 samples, u32 dc_offs);


#ifdef __cplusplus
}
#endif

#endif /* PIMP_MIXER_H */
