/* pimp_effects.h -- Implementations of the actual effects
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_EFFECTS_H
#define PIMP_EFFECTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pimp_internal.h"

/* need to move these to a separate channel state header (?) */
STATIC void porta_up(pimp_channel_state *chan, s32 period_low_clamp)
{
	ASSERT(chan != 0);

	chan->final_period -= chan->porta_speed;
	if (chan->final_period < period_low_clamp) chan->final_period = period_low_clamp;
}

STATIC void porta_down(pimp_channel_state *chan, s32 period_high_clamp)
{
	ASSERT(chan != 0);

	chan->final_period += chan->porta_speed;
	if (chan->final_period > period_high_clamp) chan->final_period = period_high_clamp;
}

STATIC void porta_note(pimp_channel_state *chan)
{
	ASSERT(chan != 0);

	if (chan->final_period > chan->porta_target)
	{
		chan->final_period -= chan->porta_speed;
		if (chan->final_period < chan->porta_target) chan->final_period = chan->porta_target;
	}
	else if (chan->final_period < chan->porta_target)
	{
		chan->final_period += chan->porta_speed;
		if (chan->final_period > chan->porta_target) chan->final_period = chan->porta_target;
	}
}

#ifdef __cplusplus
}
#endif

#endif /* PIMP_EFFECTS_H */
