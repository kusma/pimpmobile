#ifndef PIMP_RENDER
#define PIMP_RENDER

#include <gba_base.h>
#include "internal.h"

typedef struct
{
	pimp_channel_state_t channels[CHANNELS];
	u32 tick_len;
	u32 curr_tick_len;
	u32 curr_row;
	u32 curr_order;
	u32 curr_bpm;
	u32 curr_tempo;
	u32 curr_tick;
	s32 global_volume; /* 24.8 fixed point */
	
	pimp_pattern_t *curr_pattern;
	
	const unsigned char *sample_bank;
	const pimp_module_t *mod;
} pimp_mod_context;

void init_pimp_mod_context(pimp_mod_context *ctx, const pimp_module_t *mod, const u8 *sample_bank);
void pimp_render(pimp_mod_context &ctx, s8 *buf, u32 samples);

#endif /* PIMP_RENDER */
