/* load_mod.c -- Protracker MOD loading code
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "load_module.h"
#include "convert_sample.h"
#include "pimp_module.h"
#include "pimp_mixer.h" /* for pimp_loop_type enum */
#include "pimp_sample_bank.h"

#if 0
float log2(float f)
{
	return log(f) / log(2.0f);
}
#endif

int return_nearest_note(int p)
{
	double log2p, note;
	int note_int;
	
	if (p < 14) return 0; /* this is not a note */

	log2p = log(p) / log(2);
	note = (log2(428 << 5) - log2p) * 12.0 + 1;
	
	note_int = (int)floor(note + 0.5);

	if (note_int <= 0 || note_int > 120) return 0; /* this is not a note */
	return note_int;
}

static BOOL load_instrument(FILE *fp, struct pimp_instrument *instr, struct pimp_sample_bank *sample_bank)
{
	unsigned char buf[256];
	struct pimp_sample *sample;
		
	fread(buf, 1, 22, fp);
	buf[22] = '\0';

	sample = malloc(sizeof(struct pimp_sample));
	if (NULL == sample) return FALSE;

	/* only one sample per instrument in MOD */
	instr->sample_count = 1;
	
	memset(sample, 0, sizeof(struct pimp_sample));
	pimp_set_ptr(&instr->sample_ptr, sample);
	
	
	/* fill out the instrument-data. this is not stored in the module at all. */
/*	strcpy(instr->name, (const char*)buf); */
	pimp_set_ptr(&instr->vol_env_ptr, NULL);
	pimp_set_ptr(&instr->pan_env_ptr, NULL);
	instr->volume_fadeout = 0;
#if 0
	instr.new_note_action        = NNA_CUT; /* "emulate" PT2-behavour on IT-fields */
	instr.duplicate_check_type   = DCT_OFF;
	instr.duplicate_check_action = DCA_CUT;
	instr.pitch_pan_separation   = 0;
	instr.pitch_pan_center       = 0;
#endif
	memset(instr->sample_map, 0, 120);
	
/*	strcpy(samp->name, (const char*)buf); */
/*	sample->sustain_loop_type  = LOOP_NONE;
	sample->sustain_loop_start = 0;
	sample->sustain_loop_end   = 0; */
	sample->pan = 0;
/*	sample->ignore_default_pan_position = true; */
	sample->rel_note = 0;
	sample->vibrato_speed = 0;
	sample->vibrato_depth = 0;
	sample->vibrato_sweep = 0;
	sample->vibrato_waveform = SAMPLE_VIBRATO_SINE; /* just a dummy */
	
	fread(buf, 1, 2, fp);
	sample->length = ((buf[0] << 8) | buf[1]) << 1;
	
	fread(buf, 1, 1, fp);
	if (buf[0] > 7) sample->fine_tune = (buf[0] - 16) << 4;
	else sample->fine_tune = buf[0] << 4;
	
	fread(&sample->volume, 1, 1, fp);
	
	fread(buf, 1, 2, fp);
	sample->loop_start = ((buf[0] << 8) | buf[1]) << 1;
	if (sample->loop_start > sample->length) sample->loop_start = 0;
	
	fread(buf, 1, 2, fp);
	sample->loop_length = ((buf[0] << 8) | buf[1]) << 1;
	if (sample->loop_start + sample->loop_length > sample->length) sample->loop_length = sample->length - sample->loop_start;
	
	if ((sample->loop_start <= 2) && (sample->loop_length <= 4)) sample->loop_type = LOOP_TYPE_NONE;
	else sample->loop_type = LOOP_TYPE_FORWARD;
		
	return TRUE;
}


#define MAKE_WORD(a,b,c,d) ((((u32)(a)) << 24) | (((u32)(b)) << 16) | (((u32)(c)) << 8) | ((u32)(d)))

static int get_channel_count(u32 sig)
{
	int c = -1;
	switch (sig)
	{
		case MAKE_WORD('1', 'C', 'H', 'N'): c =  1; break;
		case MAKE_WORD('2', 'C', 'H', 'N'): c =  2; break;
		case MAKE_WORD('3', 'C', 'H', 'N'): c =  3; break;
		case MAKE_WORD('4', 'C', 'H', 'N'): c =  4; break;
		case MAKE_WORD('5', 'C', 'H', 'N'): c =  5; break;
		case MAKE_WORD('6', 'C', 'H', 'N'): c =  6; break;
		case MAKE_WORD('7', 'C', 'H', 'N'): c =  7; break;
		case MAKE_WORD('8', 'C', 'H', 'N'): c =  8; break;
		case MAKE_WORD('9', 'C', 'H', 'N'): c =  9; break;
		case MAKE_WORD('1', '0', 'C', 'H'): c = 10; break;
		case MAKE_WORD('1', '1', 'C', 'H'): c = 11; break;
		case MAKE_WORD('1', '2', 'C', 'H'): c = 12; break;
		case MAKE_WORD('1', '3', 'C', 'H'): c = 13; break;
		case MAKE_WORD('1', '4', 'C', 'H'): c = 14; break;
		case MAKE_WORD('1', '5', 'C', 'H'): c = 15; break;
		case MAKE_WORD('1', '6', 'C', 'H'): c = 16; break;
		case MAKE_WORD('1', '7', 'C', 'H'): c = 17; break;
		case MAKE_WORD('1', '8', 'C', 'H'): c = 18; break;
		case MAKE_WORD('1', '9', 'C', 'H'): c = 19; break;
		case MAKE_WORD('2', '0', 'C', 'H'): c = 20; break;
		case MAKE_WORD('2', '1', 'C', 'H'): c = 21; break;
		case MAKE_WORD('2', '2', 'C', 'H'): c = 22; break;
		case MAKE_WORD('2', '3', 'C', 'H'): c = 23; break;
		case MAKE_WORD('2', '4', 'C', 'H'): c = 24; break;
		case MAKE_WORD('2', '5', 'C', 'H'): c = 25; break;
		case MAKE_WORD('2', '6', 'C', 'H'): c = 26; break;
		case MAKE_WORD('2', '7', 'C', 'H'): c = 27; break;
		case MAKE_WORD('2', '8', 'C', 'H'): c = 28; break;
		case MAKE_WORD('2', '9', 'C', 'H'): c = 29; break;
		case MAKE_WORD('M', '.', 'K', '.'): c =  4; break;
		case MAKE_WORD('M', '!', 'K', '!'): c =  4; break;
		case MAKE_WORD('F', 'L', 'T', '4'): c =  4; break;
		case MAKE_WORD('C', 'D', '8', '1'): c =  8; break;
	}	
	return c;
}

pimp_module *load_module_mod(FILE *fp, struct pimp_sample_bank *sample_bank)
{
	int i, p;
	pimp_module *mod;
	int channel_count;
	int max_pattern;
	
	u8 sig_data[4];
	u32 sig;
	
	fseek(fp, 1080, SEEK_SET);
	
	fread(&sig_data, 1, 4, fp);
	sig = MAKE_WORD(sig_data[0], sig_data[1], sig_data[2], sig_data[3]);
	channel_count = get_channel_count(sig);
	if (channel_count <= 0) return NULL;
	
	mod = malloc(sizeof(pimp_module));
	if (NULL == mod) return NULL;
	
	memset(mod, 0, sizeof(pimp_module));
	
	mod->period_low_clamp           = 113; /* B-3 in MOD */
	mod->period_high_clamp          = 856; /* C-1 in MOD */
	
	/* this should be correct if no notes outside this range is used */
	mod->period_low_clamp           = 108; /* B-3 with fine tune 8 in MOD */
	mod->period_high_clamp          = 907; /* C-1 wuth fine tune -8 in MOD */
	
	mod->volume = 64;
	mod->tempo  = 6;
	mod->bpm    = 125;
	
	mod->flags = FLAG_TEMOR_EXTRA_DELAY | FLAG_TEMOR_MEMORY | FLAG_PORTA_NOTE_MEMORY;
	
	
	/* song name */
	rewind(fp);
	{
		char name[20 + 1];
		fread(name, 20, 1, fp);
		name[20] = '\0'; /* make sure name is zero-terminated */
		strcpy(mod->name, name);
	}

	/* setup channel-settings */
	{
		struct pimp_channel *channels;
		mod->channel_count = channel_count;
		
		/* allocate channel array */
		channels = malloc(sizeof(struct pimp_channel) * mod->channel_count);
		if (NULL == channels) return NULL;
		
		memset(channels, 0, sizeof(struct pimp_channel) * mod->channel_count);
		
		pimp_set_ptr(&mod->channel_ptr, channels);
		
		/* setup default pr channel settings. */
		for (i = 0; i < mod->channel_count; ++i)
		{
			channels[i].pan = 127;
			channels[i].volume = 64;
			channels[i].mute = 0;
		}
	}
	
	/* load instruments */
	{
		struct pimp_instrument *instruments;
		mod->instrument_count = 31;
		instruments = malloc(sizeof(struct pimp_instrument) * mod->instrument_count);
		if (NULL == instruments) return NULL;
			
		memset(instruments, 0, sizeof(struct pimp_instrument) * mod->instrument_count);
			
		pimp_set_ptr(&mod->instrument_ptr, instruments);
		for (i = 0; i < mod->instrument_count; ++i)
		{
			BOOL ret = load_instrument(fp, &instruments[i], sample_bank);
			if (FALSE == ret)
			{
				/* TODO: cleanup */
				return NULL;
			}
		}
	}

	/* read order count */
	{
		/* read byte */
		unsigned char order_count;
		fread(&order_count, 1, 1, fp);
		
		/* clamp and warn */
		if (order_count > 128)
		{
			fprintf(stderr, "warning: excessive orders in module, discarding.\n");
			order_count = 128;
		}
		
		mod->order_count = order_count;
	}
	
	{
		/* allocate memory */
		unsigned char *orders = malloc(mod->order_count);
		if (NULL == orders) return NULL;
		
		memset(orders, 0, mod->order_count);
		pimp_set_ptr(&mod->order_ptr, orders);

#if 1
		/* we're assuming this byte to be repeat position, but we don't really know ;) */
		fread(&mod->order_repeat, 1, 1, fp);
		if (mod->order_repeat >= mod->order_count)
		{
			fprintf(stderr, "warning: repeating at out-of-range order, setting repeat order to 0\n");
			mod->order_repeat = 0;
		}
#else
		mod->order_repeat = 0;
		fseek(fp, 1, SEEK_CUR); /* discard unused byte (this may be repeat position, but that is impossible to tell) */
#endif
	
		max_pattern = 0;
		for (i = 0; i < mod->order_count; ++i)
		{
			fread(&orders[i], 1, 1, fp);
			if (orders[i] > max_pattern) max_pattern = orders[i];
		}
	}
	
	fseek(fp, 128 - mod->order_count, SEEK_CUR); /* discard unused orders */
	fseek(fp, 4, SEEK_CUR); /* discard mod-signature (already loaded) */
	
	/* load patterns */
	{
		/* track the min and max note. this is used to detect if the module has notes outside traditional mod-limits */
		int min_period =  99999;
		int max_period = -99999;

		struct pimp_pattern *patterns;
		mod->pattern_count = max_pattern + 1;
		
		patterns = malloc(sizeof(struct pimp_pattern) * mod->pattern_count);
		if (NULL == patterns) return NULL;
		
		memset(patterns, 0, sizeof(struct pimp_pattern) * mod->pattern_count);
		pimp_set_ptr(&mod->pattern_ptr, patterns);
		
		/* load patterns  */
		for (p = 0; p < mod->pattern_count; ++p)
		{
			struct pimp_pattern_entry *pattern_data;
			
			struct pimp_pattern *pat = &patterns[p];
			pat->row_count = 64;
	
			pattern_data = malloc(sizeof(struct pimp_pattern_entry) * mod->channel_count * pat->row_count);
			if (NULL == pattern_data) return NULL;
			pimp_set_ptr(&pat->data_ptr, pattern_data);
	
			/* clear memory */
			memset(pattern_data, 0, sizeof(struct pimp_pattern_entry) * mod->channel_count * pat->row_count);
			
			for (i = 0; i < 64; ++i)
			{
				int j;
				for (j = 0; j < mod->channel_count; ++j)
				{
					unsigned char buf[4];
					int period;
					
					struct pimp_pattern_entry *pe = &pattern_data[i * mod->channel_count + j];
					fread(buf, 1, 4, fp);
					period = ((buf[0] & 0x0F) << 8) + buf[1];
					
					pe->instrument       = (buf[0] & 0x0F0) + (buf[2] >> 4);
					pe->note             = return_nearest_note(period); /* - 12; */
					pe->effect_byte      = buf[2] & 0xF;
					pe->effect_parameter = buf[3];
					
					if (period > max_period) max_period = period;
					if (period < min_period) min_period = period;
				}
			}
		}
		
		/* if there are periods in the file outside the default pt2-range, assume ft2-range */
		if (min_period < mod->period_low_clamp || max_period > mod->period_high_clamp)
		{
			mod->period_low_clamp = 1;
			mod->period_high_clamp = 32767;
		}
	}

	/* load samples */
	for (i = 0; i < 31; ++i)
	{
		struct pimp_sample *samp;
		struct pimp_instrument *instr = pimp_module_get_instrument(mod, i);
		samp = pimp_instrument_get_sample(instr, 0);
		
		if (samp->length > 0 && samp->length > 2)
		{
			enum pimp_sample_format src_format = PIMP_SAMPLE_S8;
			enum pimp_sample_format dst_format = PIMP_SAMPLE_U8;
			
			void *dst_waveform;
			void *src_waveform = malloc(samp->length * pimp_sample_format_get_size(src_format));

			if (NULL == src_waveform) return FALSE;
			fread(src_waveform, 1, samp->length * pimp_sample_format_get_size(src_format), fp);
			
			ASSERT(pimp_sample_format_get_size(src_format) == pimp_sample_format_get_size(dst_format));
			
			dst_waveform = malloc(samp->length * pimp_sample_format_get_size(dst_format));
			if (NULL == dst_waveform) return FALSE;
			
			pimp_convert_sample(dst_waveform, dst_format, src_waveform, src_format, samp->length);
			
			/* insert into sample bank. */
			{
				int pos = pimp_sample_bank_insert_sample_data(sample_bank, dst_waveform, samp->length);
				if (pos < 0)
				{
					free(dst_waveform);
					dst_waveform = NULL;

					free(src_waveform);
					src_waveform = NULL;
					
					fprintf(stderr, "failed to insert module into sample bank\n");
					return FALSE;
				}
				samp->data_ptr = pos;
			}

			free(src_waveform);
			src_waveform = NULL;
			
			free(dst_waveform);
			dst_waveform = NULL;
		}
		else
		{
			ASSERT(0); /* This is a bad fix */
			/* TODO: Handle no sample case */
			samp->data_ptr = 0;
		}
	}
	return mod;
}
