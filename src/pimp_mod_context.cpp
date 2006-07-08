#include "pimp_mod_context.h"

void __pimp_mod_context_init(pimp_mod_context *ctx, const pimp_module *mod, const u8 *sample_bank, pimp_mixer *mixer)
{
	ASSERT(ctx != NULL);
	
	ctx->mod = mod;
	ctx->sample_bank = sample_bank;
	ctx->mixer = mixer;
	
	/* setup default player-state */
	ctx->tick_len      = 0;
	ctx->curr_tick_len = 0;
	
	ctx->curr_row      = 0;
	ctx->curr_order    = 0;
	ctx->curr_pattern  = 0;
	ctx->curr_tick     = 0;
	
	ctx->curr_bpm   = 125;
	ctx->curr_tempo = 5;
	
	ctx->global_volume = 1 << 9; /* 24.8 fixed point */
	
	ctx->curr_pattern = __pimp_module_get_pattern(mod, __pimp_module_get_order(mod, ctx->curr_order));
	__pimp_mod_context_set_bpm(ctx, ctx->mod->bpm);
	ctx->curr_tempo = mod->tempo;
	
	for (unsigned i = 0; i < CHANNELS; ++i)
	{
		pimp_channel_state *chan = &ctx->channels[i];
		chan->instrument  = (const pimp_instrument*)NULL;
		chan->sample      = (const pimp_sample*)    NULL;
		chan->vol_env.env = (const pimp_envelope*)  NULL;
		__pimp_envelope_reset(&chan->vol_env);
	}
	
	ctx->callback = (pimp_callback)NULL;

	__pimp_mixer_reset(ctx->mixer);
}

void __pimp_mod_context_set_bpm(pimp_mod_context *ctx, int bpm)
{
	ASSERT(ctx != NULL);
	ASSERT(bpm > 0);
	
	/* we're using 8 fractional-bits for the tick-length */
	ctx->tick_len = int((SAMPLERATE * 5) * (1 << 8)) / (bpm * 2);
}
