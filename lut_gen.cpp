/* lut_gen.cpp -- Look-up table generator for Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

/*
	This is the lut-generator for pimpmobile.
	
	the delta-luts are dependant on the sample-rate, and must be re-generated
	whenever the sample-rate-config has changed.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <typeinfo>

#include "src/pimp_config.h" // get the current config
#include "src/pimp_math.h"

void error(const char *reason)
{
	fprintf(stderr, "error %s\n", reason);
	exit(1);
}

unsigned short linear_freq_lut[12 * 64];
unsigned short amiga_freq_lut[(AMIGA_DELTA_LUT_SIZE / 2) + 1];

float get_normal_noise()
{
	float r = 0.0f;
	for (unsigned j = 0; j < 12; ++j)
	{
		r += rand();
	}
	r /= RAND_MAX;
	r -= 6;
	return r;
}

template <typename T>
void print_lut(FILE *fp, T *lut, size_t size)
{
	int line_start = ftell(fp);
	for (size_t i = 0; i < size; ++i)
	{
		fprintf(fp, "%d, ", lut[i]);
		
		// some newlines every now and then isn't too annoying ;)
		if (ftell(fp) > (line_start + 80))
		{
			fprintf(fp, "\n\t", line_start, ftell(fp));
			line_start = ftell(fp);
		}
	}
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define PRINT_LUT(fp, x) print_lut(fp, x, ARRAY_SIZE(x))

int main(int argc, char *argv[])
{

#if 0
	{
		/* generate amiga period table */
		for (unsigned o = 0; o < 1; ++o)
		{
			for (unsigned n = 0; n < 2; ++n)
			{
				for (int fine_tune = -8; fine_tune < 8; ++fine_tune)
				{
					printf("%d, ", __pimp_get_amiga_period(n + o * 12, fine_tune) / 4);
				}
			}
			printf("\n");
		}
		return 0;
	}
#endif

	{
		// generate a lut for linear frequencies
		for (unsigned i = 0; i < 12 * 64; ++i)
		{
			linear_freq_lut[i] = unsigned(float(pow(2.0, i / 768.0) * 8363.0 / (1 << 8)) * float(1 << 9) + 0.5);
		}
		
		// now dump it
		FILE *fp = fopen("src/linear_delta_lut.h", "wb");
		if (!fp) error("failed to open out-file");
		fprintf(fp, "const u16 linear_delta_lut[%d] =\n{\n\t", ARRAY_SIZE(linear_freq_lut));
		PRINT_LUT(fp, linear_freq_lut);
		fprintf(fp, "\n};\n\n");
		fclose(fp);
	}

	{
		// generate a lut for amiga frequencies
		for (unsigned i = 0; i < (AMIGA_DELTA_LUT_SIZE / 2) + 1; ++i)
		{
			unsigned p = i + (AMIGA_DELTA_LUT_SIZE / 2);
			amiga_freq_lut[i] = (unsigned short)(((8363 * 1712) / float((p * 32768) / AMIGA_DELTA_LUT_SIZE)) * (1 << 6) + 0.5);
		}
		
		// now dump it
		FILE *fp = fopen("src/amiga_delta_lut.h", "wb");
		if (!fp) error("failed to open out-file");
		fprintf(fp, "const u16 amiga_delta_lut[%d] =\n{\n\t", ARRAY_SIZE(amiga_freq_lut));
		PRINT_LUT(fp, amiga_freq_lut);
		fprintf(fp, "\n};\n\n");
		fclose(fp);
	}

#if 0
	// testcode for amiga frequency-lut
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
		
		for (int fine_tune = -64; fine_tune < 64; fine_tune += 16)
		{
			int period = (10 * 12 * 16 * 4) - i * (16 * 4) - fine_tune / 2;
			
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
