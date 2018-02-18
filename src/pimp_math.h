/* pimp_math.h -- Math routines for use in Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_MATH_H
#define PIMP_MATH_H

#include "pimp_config.h"
#include "pimp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char pimp_clz_lut[256];

static INLINE unsigned pimp_clz16(unsigned input)
{
	/* one iteration of binary search */
	unsigned c = 0;
	
	if (input & 0xFF00) input >>= 8;
	else c += 8;
	
	/* a 256 entries lut ain't too bad... */
	return pimp_clz_lut[input] + c;
}

#ifndef NO_LINEAR_PERIODS
unsigned pimp_get_linear_delta(unsigned int period, unsigned int delta_scale);
unsigned pimp_get_linear_period(int note, int fine_tune);
#endif

#ifndef NO_AMIGA_PERIODS
unsigned pimp_get_amiga_delta(unsigned period, unsigned int delta_scale);
unsigned pimp_get_amiga_period(int note, int fine_tune);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PIMP_MATH_H */
