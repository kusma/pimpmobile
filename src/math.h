#ifndef MATH_H
#define MATH_H

#include "config.h"

extern const unsigned char clz_lut[256];

static inline unsigned clz(unsigned input)
{
	/* two iterations of binary search */
	unsigned c = 0;
	if (input & 0xFFFF0000) input >>= 16;
	else c = 16;

	if (input & 0xFF00) input >>= 8;
	else c += 8;

	/* a 256 entries lut ain't too bad... */
	return clz_lut[input] + c;
}

static inline unsigned clz16(unsigned input)
{
	/* one iteration of binary search */
	unsigned c = 0;
	
	if (input & 0xFF00) input >>= 8;
	else c += 8;

	/* a 256 entries lut ain't too bad... */
	return clz_lut[input] + c;
}

static inline unsigned clz8(unsigned input)
{
	return clz_lut[input];
}


#ifndef NO_LINEAR_PERIODS
// extern const unsigned short linear_freq_lut[12 * 64];
unsigned get_linear_delta(unsigned period);
unsigned get_linear_period(int note, int fine_tune);
#endif

#ifndef NO_AMIGA_PERIODS
extern const unsigned short amiga_freq_lut[(AMIGA_DELTA_LUT_SIZE / 2) + 1];
unsigned get_amiga_delta(unsigned period);
unsigned get_amiga_period(int note, int fine_tune);
#endif

#endif /* MATH_H */
