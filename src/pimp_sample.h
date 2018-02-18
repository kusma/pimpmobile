/* pimp_sample.h -- sample structure for Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_SAMPLE_H
#define PIMP_SAMPLE_H


typedef enum
{
	SAMPLE_VIBRATO_SINE,
	SAMPLE_VIBRATO_RAMP_DOWN,
	SAMPLE_VIBRATO_SQUARE,
	SAMPLE_VIBRATO_RANDOM, /* supported by impulse-tracker but not FT2 */
	SAMPLE_VIBRATO_RAMP_UP /* supported by FT2 but not impulse-tracker */
} pimp_vibrato_waveform;


struct pimp_sample
{
	/* offset relative to sample-bank */
	pimp_rel_ptr data_ptr;
	
	u32 length;
	u32 loop_start;
	u32 loop_length;
	
/*
	IT ONLY (later)
	u32 sustain_loop_start;
	u32 sustain_loop_end;
*/
	s16 fine_tune;
	s16 rel_note;
	
	u8  volume;
	u8  loop_type;
	u8  pan;
	
	u8 vibrato_speed;
	u8 vibrato_depth;
	u8 vibrato_sweep;
	u8 vibrato_waveform;
	
/*
	IT ONLY (later)
	u8  sustain_loop_type;
*/
};


#endif /* PIMP_SAMPLE_H */
