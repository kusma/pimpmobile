#ifndef PIMP_MOD_CONTEXT_H
#define PIMP_MOD_CONTEXT_H

#include "../include/pimpmobile.h"
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
	
	pimp_callback callback;
} pimp_mod_context;

void __pimp_mod_context_init(pimp_mod_context *ctx, const pimp_module *mod, const u8 *sample_bank, pimp_mixer *mixer);
void __pimp_mod_context_set_pos(pimp_mod_context *ctx, int row, int order);
void __pimp_mod_context_set_bpm(pimp_mod_context *ctx, int bpm);
void __pimp_mod_context_set_tempo(pimp_mod_context *ctx, int tempo);

#endif /* PIMP_MOD_CONTEXT_H */
