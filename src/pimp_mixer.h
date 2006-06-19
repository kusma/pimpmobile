#ifndef PIMP_MIXER_H
#define PIMP_MIXER_H

#include "pimp_types.h"
#include "config.h"

typedef enum
{
	LOOP_TYPE_NONE,
	LOOP_TYPE_FORWARD,
	LOOP_TYPE_PINGPONG
} pimp_mixer_loop_type;

typedef struct
{
	u32                   sample_length;
	u32                   loop_start;
	u32                   loop_end;
	pimp_mixer_loop_type  loop_type;
	const u8             *sample_data;
	u32                   sample_cursor;
	s32                   sample_cursor_delta;
	s32                   volume;
} pimp_mixer_channel_state;

typedef struct
{
	pimp_mixer_channel_state channels[CHANNELS];
	/* TODO: any other needed states */
} pimp_mixer;
	
void pimp_mixer_reset(pimp_mixer *mixer);
void pimp_mixer_mix(pimp_mixer *mixer, s8 *target, int samples);

u32  pimp_mixer_mix_samples(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta);
void pimp_mixer_clip_samples(s8 *target, s32 *source, u32 samples, u32 dc_offs);

#endif /* PIMP_MIXER_H */
