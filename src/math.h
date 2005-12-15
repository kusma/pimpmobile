#ifndef _PIMP_MATH_H_
#define _PIMP_MATH_H_

#include "config.h"

extern const unsigned char clz_lut[256];

static inline unsigned clz(unsigned input)
{
	/* 2 iterations of binary search */
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
	/* 1 iteration of binary search */
	unsigned c = 0;
	
	if (input & 0xFF00) input >>= 8;
	else c += 8;

	/* a 256 entries lut ain't too bad... */
	return clz_lut[input] + c;
}

extern unsigned short linear_freq_lut[12 * 64];
static inline unsigned get_linear_delta(unsigned period)
{
	unsigned p = (12 * 64 * 14) - period;
	unsigned octave        = p / (12 * 64);
	unsigned octave_period = p % (12 * 64);
	unsigned frequency = linear_freq_lut[octave_period] << octave;

	// BEHOLD: the expression of the devil 2.0
	frequency = ((long long)frequency * unsigned((1.0 / SAMPLERATE) * (1 << 3) * (1ULL << 32)) + (1ULL << 31)) >> 32;
	return frequency;
}

extern unsigned short amiga_freq_lut[(AMIGA_FREQ_TABLE_SIZE / 2) + 1];
unsigned get_amiga_delta(unsigned period);

#endif /* _PIMP_MATH_H_ */
