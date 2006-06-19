#ifndef PIMP_RENDER
#define PIMP_RENDER

#include <gba_base.h>
#include "pimp_internal.h"
#include "pimp_module.h"
#include "pimp_mixer.h"

typedef struct
{
	u32 tick_len;
	u32 curr_tick_len;
	u32 curr_row;
	u32 curr_order;
	u32 curr_bpm;
	u32 curr_tempo;
	u32 curr_tick;
	s32 global_volume; /* 24.8 fixed point */
	pimp_pattern *curr_pattern;
	pimp_channel_state channels[CHANNELS];
	
	const u8          *sample_bank;
	const pimp_module *mod;
	pimp_mixer        *mixer;
	
} pimp_mod_context;

void pimp_mod_context_init(pimp_mod_context *ctx, const pimp_module *mod, const u8 *sample_bank, pimp_mixer *mixer);
void pimp_render(pimp_mod_context *ctx, s8 *buf, u32 samples);

#endif /* PIMP_RENDER */
