#include "pimp_envelope.h"
#include "pimp_debug.h"

#include <stdio.h>

#if 0
unsigned eval_vol_env(pimp_channel_state &chan)
{
	ASSERT(NULL != chan.vol_env);

#if 0
	/* find the current volume envelope node */
	if (chan.vol_env_node = -1)
	{
		for (int i = 0; i < chan.vol_env->node_count - 1; ++i)
		{
			if (chan.vol_env->node_tick[i] <= chan.vol_env_tick)
			{
				chan.vol_env_node = i;
				break;
			}
		}
	}
#endif

	// TODO: sustain
	
	// the magnitude of the envelope at tick N:
	// first, find the last node at or before tick N - its position is M
	// then, the magnitude of the envelope at tick N is given by
	// magnitude = node_magnitude[M] + ((node_delta[M] * (N-M)) >> 8)
	
	s32 delta = chan.vol_env->node_delta[chan.vol_env_node];
	u32 internal_tick = chan.vol_env_tick - chan.vol_env->node_tick[chan.vol_env_node];
	
	int val = chan.vol_env->node_magnitude[chan.vol_env_node];
	val += ((long long)delta * internal_tick) >> 9;
	
	chan.vol_env_tick++;

	if (chan.vol_env_node < (chan.vol_env->node_count - 1))
	{
#if 0
		if (chan.vol_env->flags & (1 << 1))
		{
			/* sustain loop */
			chan.vol_env_tick = 0; // HACK: just used to make the BP06 demo tune play correctly
		}
		else 
#endif
		if (chan.vol_env_tick >= chan.vol_env->node_tick[chan.vol_env_node + 1])
		{
			chan.vol_env_node++;
		}
	}
	
	return val << 2;
}
#endif

int __pimp_envelope_sample(pimp_envelope_state *state)
{
	ASSERT(NULL != state);

	// the magnitude of the envelope at tick N:
	// first, find the last node at or before tick N - its position is M
	// then, the magnitude of the envelope at tick N is given by
	// magnitude = node_magnitude[M] + ((node_delta[M] * (N-M)) >> 8)
	
	s32 delta = state->env->node_delta[state->current_node];
	u32 internal_tick = state->current_tick - state->env->node_tick[state->current_node];
	
	int val = state->env->node_magnitude[state->current_node];
	val += ((long long)delta * internal_tick) >> 9;
	
	return val << 2;
}

void __pimp_envelope_advance_tick(pimp_envelope_state *state, bool sustain)
{
	ASSERT(NULL != state);
	
	// advance a tick (TODO: move to separate function)
	state->current_tick++;
	iprintf("%d\n", state->current_tick);
	if (state->current_node < (state->env->node_count - 1))
	{
		// TODO: sustain
#if 0
		if (chan.vol_env->flags & (1 << 1))
		{
			/* sustain loop */
			chan.vol_env_tick = 0; // HACK: just used to make the BP06 demo tune play correctly
		}
		else 
#endif
		if (state->current_tick >= state->env->node_tick[state->current_node + 1])
		{
			state->current_node++;
		}
	}
}
