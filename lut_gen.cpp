#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "src/config.h" // get the current config

const unsigned char clz_lut[256] =
{
	0x8, 0x7, 0x6, 0x6, 0x5, 0x5, 0x5, 0x5, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};

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

unsigned short linear_freq_lut[12 * 64];

unsigned get_linear_delta(unsigned period)
{
	unsigned p = (12 * 64 * 14) - period;
	unsigned octave        = p / (12 * 64);
	unsigned octave_period = p % (12 * 64);
	unsigned frequency = linear_freq_lut[octave_period] << octave;

	// BEHOLD: the expression of the devil 2.0
	frequency = ((long long)frequency * unsigned((1.0 / SAMPLERATE) * (1 << 3) * (1ULL << 32)) + (1ULL << 31)) >> 32;
	return frequency;
}

#define AMIGA_FREQ_TABLE_LOG_SIZE 7
#define AMIGA_FREQ_TABLE_SIZE (1 << AMIGA_FREQ_TABLE_LOG_SIZE)
#define AMIGA_FREQ_TABLE_FRAC_BITS (15 - AMIGA_FREQ_TABLE_LOG_SIZE)
unsigned short amiga_freq_lut[(AMIGA_FREQ_TABLE_SIZE / 2) + 1];

unsigned get_amiga_delta(unsigned period)
{
	unsigned shamt = clz16(period) - 1;
	unsigned p = period << shamt;
	unsigned p_frac = p & ((1 << AMIGA_FREQ_TABLE_FRAC_BITS) - 1);
	p >>= AMIGA_FREQ_TABLE_FRAC_BITS;

	// interpolate table-entries for better result
	int f1 = amiga_freq_lut[p     - (AMIGA_FREQ_TABLE_SIZE / 2)]; // (8363 * 1712) / float(p);
	int f2 = amiga_freq_lut[p + 1 - (AMIGA_FREQ_TABLE_SIZE / 2)]; // (8363 * 1712) / float(p + 1);
	unsigned frequency = (f1 << AMIGA_FREQ_TABLE_FRAC_BITS) + (f2 - f1) * p_frac;

	if (shamt > AMIGA_FREQ_TABLE_FRAC_BITS) frequency <<= shamt - AMIGA_FREQ_TABLE_FRAC_BITS;
	else frequency >>= AMIGA_FREQ_TABLE_FRAC_BITS - shamt;

	// BEHOLD: the expression of the devil
	// quasi-explaination:     the table-lookup  the playback freq  - the LUT presc - make it overflow - round it - pick the top
	frequency = ((long long)frequency * unsigned(((1.0 / SAMPLERATE) * (1 << 6)) * (1LL << 32)) + (1ULL << 31)) >> 32;
	return frequency;
}

float get_normal_noise()
{
	float r = 0.0;
	for (unsigned j = 0; j < 12; ++j)
	{
		r += rand();
	}
	r /= RAND_MAX;
	r -= 6;
	return r;
}

int main(int argc, char *argv[])
{
/*
	for (unsigned i = 32; i < 255; ++i)
	{
		printf("%f\n", (18157 * 5) / float(i * 2));
	}
*/

	// generate a lut for linear frequencies
	for (unsigned i = 0; i < 12 * 64; ++i)
	{
		linear_freq_lut[i] = unsigned(float(pow(2.0, i / 768.0) * 8363.0 / (1 << 8)) * float(1 << 9) + 0.5);
	}

	// generate a lut for amiga frequencies
	for (unsigned i = 0; i < (AMIGA_FREQ_TABLE_SIZE / 2) + 1; ++i)
	{
		unsigned p = i + (AMIGA_FREQ_TABLE_SIZE / 2);
		amiga_freq_lut[i] = (unsigned short)(((8363 * 1712) / float((p * 32768) / AMIGA_FREQ_TABLE_SIZE)) * (1 << 6) + 0.5);
	}
#if 1
	for (unsigned period = 1; period < 32767; period += 17)
	{
		float frequency1 = (8363 * 1712) / float(period);
		float delta1 = frequency1 / SAMPLERATE;
		delta1 = unsigned(delta1 * (1 << 12) + 0.5) * (1.0 / (1 << 12));
		
		float delta2 = get_amiga_delta(period) * (1.0 / (1 << 12));
		
//		printf("%f %f\n", delta1, delta2);
		printf("%f %f, %f\n", delta1, delta2, fabs(delta1 - delta2) / delta1);
	}
#endif

#if 0 // testcode for linear frequency-lut
	for (unsigned i = 0; i < 12 * 14; ++i)
	{
//		Period = 10*12*16*4 - Note*16*4 - FineTune/2;
//		Frequency = 8363*2^((6*12*16*4 - Period) / (12*16*4));
		
		for (int finetune = -64; finetune < 64; finetune += 16)
		{
			int period = (10 * 12 * 16 * 4) - i * (16 * 4) - finetune / 2;
			
			float frequency1 = 8363 * pow(2.0, float(6 * 12 * 16 * 4 - period) / (12 * 16 * 4));
			float delta1 = frequency1 / SAMPLERATE;
			delta1 = unsigned(delta1 * (1 << 12) + 0.5) * (1.0 / (1 << 12));
			
			float delta2 = get_linear_delta(period) * (1.0 / (1 << 12));
			
			printf("%f ", (delta1 - delta2) / delta1);
//			printf("%f\n", delta1);
		}
		printf("\n");
	}
#endif

#if 0
	for (unsigned i = 0; i < 304; ++i)
	{
		float r;
		
		do {
			r = get_normal_noise() * (2.0 / 3) * (1 << 6);
		}
		while (r > 127 || r < -128);
		
		printf("%i, ", int(r));
	}
#endif
}
