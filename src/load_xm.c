/* load_xm.c -- FastTracker XM loading code
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "load_module.h"
#include "convert_sample.h"
#include "pimp_module.h"
#include "pimp_mixer.h" /* for pimp_loop_type enum */
#include "pimp_sample_bank.h"

typedef struct
{
	uint32_t header_size;
	uint16_t len;
	uint16_t restart_pos;
	uint16_t channels;
	uint16_t patterns;
	uint16_t instruments;
	uint16_t flags;
	uint16_t tempo;
	uint16_t bpm;
} xm_header;

typedef struct
{
	uint32_t header_size;
	uint8_t  packing_type;
	uint16_t rows;
	uint16_t data_size;
} xm_pattern_header;

typedef struct
{
	/* part 1 */
	uint32_t header_size;
	char     name[22 + 1];
	uint8_t  type;
	uint16_t samples;
	
	/* part 2 */
	uint32_t sample_header_size;
	uint8_t  sample_number[96];
	uint16_t vol_env[48];
	uint16_t pan_env[48];
	uint8_t  vol_env_points;
	uint8_t  pan_env_points;
	uint8_t  vol_sustain;
	uint8_t  vol_loop_start;
	uint8_t  vol_loop_end;
	uint8_t  pan_sustain;
	uint8_t  pan_loop_start;
	uint8_t  pan_loop_end;
	uint8_t  vol_type;
	uint8_t  pan_type;
	uint8_t  vibrato_type;
	uint8_t  vibrato_sweep;
	uint8_t  vibrato_depth;
	uint8_t  vibrato_rate;
	uint16_t volume_fadeout;
} xm_instrument_header;

typedef struct
{
	uint32_t length;
	uint32_t loop_start;
	uint32_t loop_length;
	uint8_t  volume;
	int8_t   fine_tune;
	uint8_t  type;
	uint8_t  pan;
	int8_t   rel_note;
	char     name[22 + 1];
} xm_sample_header;

static BOOL read_byte(uint8_t *dst, FILE *fp)
{
	return fread(dst, 1, 1, fp) == 1;
}

static BOOL read_word(uint16_t *dst, FILE *fp)
{
	return fread(dst, 2, 1, fp) == 1;
}

static BOOL read_dword(uint32_t *dst, FILE *fp)
{
	return fread(dst, 4, 1, fp) == 1;
}

static BOOL read_string(char *dst, int size, FILE *fp)
{
	if (fread(dst, size - 1, 1, fp) != 1)
		return FALSE;

	dst[size - 1] = '\0';
	return TRUE;
}

static BOOL load_instrument(FILE *fp, struct pimp_instrument *instr, struct pimp_sample_bank *sample_bank)
{
	int i, s;
	size_t last_pos;
	xm_instrument_header ih;
	xm_sample_header *sample_headers;
	
	ASSERT(NULL != fp);
	ASSERT(NULL != instr);
	
	last_pos = ftell(fp);
	
	memset(instr, 0, sizeof(struct pimp_instrument));
	
	/* clear structure */
	memset(&ih, 0, sizeof(ih));
	
	/* read part 1 */
	if (!read_dword(&ih.header_size, fp) ||
	    !read_string(ih.name, sizeof(ih.name), fp) ||
	    !read_byte(&ih.type, fp) ||
	    !read_word(&ih.samples, fp))
		return FALSE;

	if (ih.samples != 0)
	{
		/* read part 2 */
		if (!read_dword(&ih.sample_header_size, fp) ||
		    fread(&ih.sample_number,      1, 96, fp) != 96 ||
		    fread(&ih.vol_env,            2, 24, fp) != 24 ||
		    fread(&ih.pan_env,            2, 24, fp) != 24 ||
		    !read_byte(&ih.vol_env_points, fp) ||
		    !read_byte(&ih.pan_env_points, fp) ||
		    !read_byte(&ih.vol_sustain,    fp) ||
		    !read_byte(&ih.vol_loop_start, fp) ||
		    !read_byte(&ih.vol_loop_end,   fp) ||
		    !read_byte(&ih.pan_sustain,    fp) ||
		    !read_byte(&ih.pan_loop_start, fp) ||
		    !read_byte(&ih.pan_loop_end,   fp) ||
		    !read_byte(&ih.vol_type,       fp) ||
		    !read_byte(&ih.pan_type,       fp) ||
		    !read_byte(&ih.vibrato_type,   fp) ||
		    !read_byte(&ih.vibrato_sweep,  fp) ||
		    !read_byte(&ih.vibrato_depth,  fp) ||
		    !read_byte(&ih.vibrato_rate,   fp) ||
		    !read_word(&ih.volume_fadeout, fp))
			return FALSE;
	}
	
	pimp_set_ptr(&instr->vol_env_ptr, NULL);
	if ((ih.vol_type & 1) == 1)
	{
		struct pimp_envelope *env = malloc(sizeof(struct pimp_envelope));
		if (NULL == env) return FALSE;
		memset(env, 0, sizeof(struct pimp_envelope));
		
		pimp_set_ptr(&instr->vol_env_ptr, env);

		env->node_count = ih.vol_env_points;
		for (i = 0; i < ih.vol_env_points; ++i)
		{
			env->node_tick[i]      = ih.vol_env[i * 2 + 0];
			env->node_magnitude[i] = ih.vol_env[i * 2 + 1];
			
			if (i > 0)
			{
				int diff_mag  = env->node_magnitude[i] - env->node_magnitude[i - 1];
				int diff_tick = env->node_tick[i] - env->node_tick[i - 1];
				
				env->node_delta[i - 1] = (diff_mag << 9) / diff_tick; /* 7.9 fixedpoint. should this be auto-calculated somewhere for all mod-types instead? */
			}
		}
		
		env->sustain_loop_start = env->sustain_loop_end = ih.vol_sustain;
		env->loop_start  = ih.vol_loop_start;
		env->loop_end    = ih.vol_loop_end;
		
/*
		env->sustain_loop_enable = (ih.vol_type & (1 << 1)) == 1;
		env->loop_enable         = (ih.vol_type & (1 << 2)) == 1;
*/
		/* env->flags:  bit 0: loop enable,     bit 1: sustain loop enable */
		/* ih.vol_type: bit 0: envelope enable, bit 1: sustain loope enable, bit 2: loop enable */
		env->flags  = (((ih.vol_type >> 1) & 1) == 1) ? (1 << 1) : 0;
		env->flags |= (((ih.vol_type >> 2) & 1) == 1) ? (1 << 0) : 0;
	}
	
	pimp_set_ptr(&instr->pan_env_ptr, NULL);
	if ((ih.pan_type & 1) == 1)
	{
		/* panning envelope */
		struct pimp_envelope *env = malloc(sizeof(struct pimp_envelope));
		if (NULL == env) return FALSE;
		memset(env, 0, sizeof(struct pimp_envelope));
		
		pimp_set_ptr(&instr->pan_env_ptr, env);
		
		env->node_count = ih.pan_env_points;
		for (i = 0; i < ih.pan_env_points; ++i)
		{
			env->node_tick[i]      = ih.pan_env[i * 2 + 0];
			env->node_magnitude[i] = ih.pan_env[i * 2 + 1];
			
			if (i > 0)
			{
				int diff_mag  = env->node_magnitude[i] - env->node_magnitude[i - 1];
				int diff_tick = env->node_tick[i] - env->node_tick[i - 1];
				
				env->node_delta[i - 1] = (diff_mag << 9) / diff_tick; /* 7.9 fixedpoint. should this be auto-calculated somewhere for all mod-types instead? */
			}
		}
		env->sustain_loop_start = env->sustain_loop_end = ih.pan_sustain;
		env->loop_start = ih.pan_loop_start;
		env->loop_end   = ih.pan_loop_end;
/*
		env->sustain_loop_enable = (ih.pan_type & (1 << 1)) == 1;
		env->loop_enable         = (ih.pan_type & (1 << 2)) == 1;
*/
		/* env->flags: bit 0: loop enable, bit 1: sustain loop enable */
		/* ih.pan_type: bit 0: sustain loope enable, bit1: loop enable */
		env->flags  = (((ih.pan_type >> 1) & 1) == 1) ? (1 << 1) : 0;
		env->flags |= (((ih.pan_type >> 2) & 1) == 1) ? (1 << 0) : 0;
	}
	
	instr->volume_fadeout = ih.volume_fadeout;
	
	/* "emulate" FT2-behavour on IT-fields */
/*
	TODO: add flags to envelope-struct, and set these 
	instr->new_note_action = NNA_CUT;
	instr->duplicate_check_type = DCT_OFF;
	instr->duplicate_check_action = DCA_CUT;
*/

	instr->sample_count = ih.samples;

	{
		struct pimp_sample *samples = NULL;
		if (instr->sample_count > 0)
		{
			samples = malloc(sizeof(struct pimp_sample) * instr->sample_count);
			if (NULL == samples) return FALSE;
		}
		pimp_set_ptr(&instr->sample_ptr, samples);
	}
	
	if (fseek(fp, ih.header_size - (ftell(fp) - last_pos), SEEK_CUR) < 0)
		return FALSE;
	
	/* sample headers allocated dynamically because data needs to be remembered. we're doing two passes here - one for sample headers, and one for sample data. */
	sample_headers = malloc(sizeof(xm_sample_header) * ih.samples);
	if (NULL == sample_headers) return FALSE;
		
	memset(sample_headers, 0, sizeof(xm_sample_header) * ih.samples);
	
	/* all sample headers are stored first, and THEN comes all sample-data. now it's time for them headers */
	for (s = 0; s < instr->sample_count; ++s)
	{
		struct pimp_sample *samp = NULL;
		xm_sample_header *sh = &sample_headers[s];
		
		/* load sample-header */
		if (!read_dword(&sh->length, fp) ||
		    !read_dword(&sh->loop_start,  fp) ||
		    !read_dword(&sh->loop_length, fp) ||
		    !read_byte(&sh->volume, fp) ||
		    fread(&sh->fine_tune, 1, 1, fp) != 1 ||
		    !read_byte(&sh->type, fp) ||
		    !read_byte(&sh->pan, fp) ||
		    fread(&sh->rel_note, 1, 1, fp) != 1 ||
		    fseek(fp, 1, SEEK_CUR) < 0 ||
		    !read_string(sh->name, sizeof(sh->name), fp))
			return FALSE;
		
		/* fill converter-struct */
		samp = pimp_instrument_get_sample(instr, s);
/*		sample_header_t &samp = instr->samples[s]; */
/*		strcpy(samp->name, sh.name); */
		
/*		samp->is_stereo = false; */ /* no stereo samples in XM */
		switch (sh->type & ((1 << 2) - 1))
		{
			case 0:
				samp->loop_type = LOOP_TYPE_NONE;
			break;
			case 1:
				samp->loop_type = LOOP_TYPE_FORWARD;
			break;
			case 2:
				samp->loop_type = LOOP_TYPE_PINGPONG;
			break;
			default: assert(0);
		}
		
		samp->loop_start  = sh->loop_start;
		samp->loop_length = sh->loop_length;

		pimp_set_ptr(&samp->data_ptr, NULL);
/*		samp->rel_ptr     = -1; */

#if 0		
		/* IT only */
		samp->sustain_loop_type = LOOP_TYPE_NONE;
		samp->sustain_loop_start = 0;
		samp->sustain_loop_end = 0;
#endif
		
		samp->volume = sh->volume;
		samp->pan    = sh->pan;
#if 0
		samp->ignore_default_pan_position = false; /* allways pan from instrument in XM */
#endif

		samp->rel_note  = sh->rel_note; /* + 12; */
		samp->fine_tune = sh->fine_tune;

#if 0
		/* glissando ? */
		samp->note_period_translation_map_index = 0;
#endif
		
		samp->vibrato_speed = ih.vibrato_rate;
		samp->vibrato_depth = ih.vibrato_depth;
		samp->vibrato_sweep = ih.vibrato_sweep;
		
		/* no documentation, this is reversed from MPT */
		switch (ih.vibrato_type)
		{
			case 0:
				samp->vibrato_waveform = SAMPLE_VIBRATO_SINE;
			break;
			case 1:
				samp->vibrato_waveform = SAMPLE_VIBRATO_SQUARE;
			break;
			case 2:
				samp->vibrato_waveform = SAMPLE_VIBRATO_RAMP_UP;
			break;
			case 3:
				samp->vibrato_waveform = SAMPLE_VIBRATO_RAMP_DOWN;
			break;
			case 4:
				samp->vibrato_waveform = SAMPLE_VIBRATO_RANDOM;
			break;
			default: assert(0);
		}
	}
	
	/* now load all samples */
	for (s = 0; s < instr->sample_count; ++s)
	{
		xm_sample_header *sh = &sample_headers[s];
		struct pimp_sample *samp = pimp_instrument_get_sample(instr, s);
		enum pimp_sample_format src_format;
		enum pimp_sample_format dst_format = PIMP_SAMPLE_U8;

		void *dst_waveform;
		void *src_waveform = malloc(sh->length);
		if (NULL == src_waveform) return FALSE;
		
		if (sh->type & (1 << 4))
		{
			signed short prev;
			
			/* signed 16 bit waveform data */
			src_format = PIMP_SAMPLE_S16;
			
			prev = 0;
			for (i = 0; i < (int)(sh->length / 2); ++i)
			{
				uint16_t data = 0;
				if (!read_word(&data, fp))
					return FALSE;
				prev += data;
				((int16_t *)src_waveform)[i] = prev;
			}
			
			/* adjust loop points */
			{
				int loop_end = samp->loop_start + sh->loop_length;
				samp->loop_start  = samp->loop_start / 2;
				samp->loop_length = (loop_end / 2) - samp->loop_start;
			}
		}
		else
		{
			unsigned i;
			signed char prev;
			
			/* signed 8 bit waveform data */
			src_format = PIMP_SAMPLE_S8;
			
			prev = 0;
			for (i = 0; i < sh->length; ++i)
			{
				uint8_t data = 0;
				if (!read_byte(&data, fp))
					return FALSE;
				prev += data;
				((int8_t *)src_waveform)[i] = prev;
			}
		}
		
		/* calculate length of sample in native format */
		samp->length = sh->length / pimp_sample_format_get_size(src_format);
		ASSERT((sh->length % pimp_sample_format_get_size(src_format)) == 0);
		
		/* allocate memory for converted sample */
		dst_waveform = malloc(samp->length * pimp_sample_format_get_size(dst_format));
		if (NULL == dst_waveform) return FALSE;

		/* convert the actual sample */
		pimp_convert_sample(dst_waveform, dst_format, src_waveform, src_format, samp->length);

		/* we're done with src_waveform */
		free(src_waveform);
		src_waveform = NULL;
		
		/* insert into sample bank. */
		{
			int pos = pimp_sample_bank_insert_sample_data(sample_bank, dst_waveform, samp->length);
			if (pos < 0)
			{
				free(dst_waveform);
				dst_waveform = NULL;
				return FALSE;
			}
			samp->data_ptr = pos;
		}

		free(dst_waveform);
		dst_waveform = NULL;
	}
	
	for (i = 0; i < 96; ++i)
	{
		instr->sample_map[i + 12] = ih.sample_number[i]; /* some offset here? */
	}
	
	return TRUE;
}

pimp_module *load_module_xm(FILE *fp, struct pimp_sample_bank *sample_bank)
{
	int i;
	pimp_module *mod;
	xm_header xm_header;
	char name[20 + 1];
	char tracker_name[20 + 1];
	
	ASSERT(NULL != sample_bank);
	
	/* check header */
	{
		char temp[17 + 1];
		rewind(fp);
		if (read_string(temp, sizeof(temp), fp) != 1 ||
		    strcmp(temp, "Extended Module: ") != 0)
			return NULL;
	}
	
	/* read song name */
	if (!read_string(name, sizeof(name), fp))
		return NULL;

	/* just another file-sanity-check */
	{
		uint8_t magic;
		if (!read_byte(&magic, fp) ||
		    magic != 0x1a)
			return NULL;
	}
	
	/* read tracker name (seems to not be very reliable for bug-emulating) */
	if (!read_string(tracker_name, sizeof(tracker_name), fp))
		return NULL;
	
	/* check version */
	{
		uint16_t version;
		if (!read_word(&version, fp))
			return NULL;

		if (version < 0x104)
		{
			fprintf(stderr, "too old file format version, please open and resave in ft2\n");
			return NULL;
		}
	}
	
	mod = malloc(sizeof(pimp_module));
	if (NULL == mod) return NULL;
	
	memset(mod, 0, sizeof(pimp_module));

	strcpy(mod->name, name);
	
	memset(&xm_header, 0, sizeof(xm_header));
	
	/* load song-header */
	if (!read_dword(&xm_header.header_size, fp) ||
	    !read_word(&xm_header.len, fp) ||
	    !read_word(&xm_header.restart_pos, fp) ||
	    !read_word(&xm_header.channels, fp) ||
	    !read_word(&xm_header.patterns, fp) ||
	    !read_word(&xm_header.instruments, fp) ||
	    !read_word(&xm_header.flags, fp) ||
	    !read_word(&xm_header.tempo, fp) ||
	    !read_word(&xm_header.bpm, fp))
		return NULL;
	
	/* this is the index of the highest pattern,
	not the number of patterns as documented.
	ohwell, let's convert. */
	
#ifdef PRINT_HEADER
	printf("header size: %u\n", xm_header.header_size);
	printf("len:         %u\n", xm_header.len);
	printf("restart pos: %u\n", xm_header.restart_pos);
	printf("channels:    %u\n", xm_header.channels);
	printf("patterns:    %u\n", xm_header.patterns);
	printf("instruments: %u\n", xm_header.instruments);
	printf("flags:       %u\n", xm_header.flags);
	printf("tempo:       %u\n", xm_header.tempo);
	printf("bpm:         %u\n", xm_header.bpm);
#endif
#if 0
	if ((xm_header.flags & 1) == 0) printf("AAAAMIIIIIGAH!\n");
	else printf("LINEAR!!\n");
#endif
	if ((xm_header.flags & 1) == 0)
	{
		mod->flags &= ~FLAG_LINEAR_PERIODS; /* mask out linear periods flag */
		mod->flags &= ~FLAG_LINEAR_VIBRATO; /* mask out linear vibrato periods flag */
/*
		mod->use_linear_frequency_table = false;            // behavour flag
		mod->instrument_vibrato_use_linear_frequency_table = false; // behavour flag
*/
		mod->period_low_clamp = 1;
		mod->period_high_clamp = 32767;
	}
	else
	{
		mod->flags |= FLAG_LINEAR_PERIODS; /* set linear periods flag */
		mod->flags |= FLAG_LINEAR_VIBRATO; /* set linear vibrato periods flag */
/*
		mod->use_linear_frequency_table = true;            // behavour flag
		mod->instrument_vibrato_use_linear_frequency_table = true; // behavour flag
*/
		mod->period_low_clamp = 0;
		mod->period_high_clamp = 10752;
	}
	
	mod->volume = 64;
	mod->tempo = xm_header.tempo;
	mod->bpm   = xm_header.bpm;
	
	/* this is behavour-flags for the player, used to flag differences between MOD, XM, S3M and IT */
	mod->flags |=  FLAG_TEMOR_EXTRA_DELAY | FLAG_TEMOR_MEMORY | FLAG_ALLOW_NESTED_LOOPS | FLAG_PORTA_NOTE_MEMORY;
/*	UNSURE, CHECK THIS ONE: mod->tone_porta_share_memory      = false; */

	mod->order_count = xm_header.len;
	{
		void *orders = malloc(mod->order_count);
		if (NULL == orders) return NULL;
		
		if (fread(orders, sizeof(uint8_t), mod->order_count, fp) != mod->order_count)
			return NULL;

		pimp_set_ptr(&mod->order_ptr, orders);
	}
	mod->order_repeat = xm_header.restart_pos;
	
	{
		struct pimp_channel *channels;
		mod->channel_count = xm_header.channels;
		channels = malloc(sizeof(struct pimp_channel) * mod->channel_count);
		if (NULL == channels) return NULL;
		
		pimp_set_ptr(&mod->channel_ptr, channels);
		
		/* setup default pr channel settings. */
		for (i = 0; i < mod->channel_count; ++i)
		{
			channels[i].pan = 127;
			channels[i].volume = 64;
			channels[i].mute = 0;
		}
	}
	
	/* seek to start of pattern */
	if (fseek(fp, 60 + xm_header.header_size, SEEK_SET) < 0)
		return NULL;
	
	{
		int p;
		struct pimp_pattern *patterns;
		
		mod->pattern_count = xm_header.patterns;
		/* allocate memory for patterns */
		patterns = malloc(sizeof(struct pimp_pattern) * mod->pattern_count);
		if (NULL == patterns) return NULL;
		
		pimp_set_ptr(&mod->pattern_ptr, patterns);
		
		/* clear memory */
		memset(patterns, 0, sizeof(struct pimp_pattern) * mod->pattern_count);
		
		
		/* load patterns */
		for (p = 0; p < xm_header.patterns; ++p)
		{
			struct pimp_pattern *pat;
			struct pimp_pattern_entry *pattern_data;
			size_t last_pos = ftell(fp);
			xm_pattern_header pattern_header;
			memset(&pattern_header, 0, sizeof(pattern_header));
			
			/* load pattern-header */
			if (!read_dword(&pattern_header.header_size, fp) ||
			    !read_byte(&pattern_header.packing_type, fp) ||
			    !read_word(&pattern_header.rows, fp) ||
			    !read_word(&pattern_header.data_size, fp))
				return NULL;
			
			pat = &patterns[p];
			pat->row_count = pattern_header.rows;
			
			/* allocate memory for pattern data */
			pattern_data = malloc(sizeof(struct pimp_pattern_entry) * mod->channel_count * pat->row_count);
			if (NULL == pattern_data)
			{
				return NULL;
			}
			pimp_set_ptr(&pat->data_ptr, pattern_data);

			/* clear memory */
			memset(pattern_data, 0, sizeof(struct pimp_pattern_entry) * mod->channel_count * pat->row_count);
			
			
#ifdef PRINT_PATTERNS
			printf("pattern:              %u\n", p);
			printf("pattern-header size:  %u\n", pattern_header.header_size);
			printf("pattern packing type: %u\n", pattern_header.packing_type);
			printf("pattern rows:         %u\n", pattern_header.rows);
			printf("pattern data size:    %u\n", pattern_header.data_size);
#endif
	
			
			/* seek to start of pattern data */
			if (fseek(fp, last_pos + pattern_header.header_size, SEEK_SET) < 0)
				return NULL;
			
			/* load pattern data */
			if (pattern_header.data_size > 0)
			{
				int r;
				for (r = 0; r < pattern_header.rows; ++r)
				{
					int n;
	#ifdef PRINT_PATTERNS
					printf("%02X - ", r);
	#endif
					for (n = 0; n < xm_header.channels; ++n)
					{
						struct pimp_pattern_entry *dat = NULL;
						uint8_t pack;
						uint8_t
							note      = 0,
							instr     = 0,
							vol       = 0,
							eff       = 0,
							eff_param = 0;
						
						if (!read_byte(&note, fp))
							return NULL;
						
						pack = 0x1E;
						if (note & (1 << 7))
						{
							pack = note;
							note = 0;
						}
						
						note &= (1 << 7) - 1; /* we never need the top-bit. */
						
						if (pack & (1 << 0))
							if (!read_byte(&note, fp))
								return NULL;
						if (pack & (1 << 1))
							if (!read_byte(&instr, fp))
								return NULL;
						if (pack & (1 << 2))
							if (!read_byte(&vol, fp))
								return NULL;
						if (pack & (1 << 3))
							if (!read_byte(&eff, fp))
								return NULL;
						if (pack & (1 << 4))
							if (!read_byte(&eff_param, fp))
								return NULL;

						dat = &pattern_data[r * mod->channel_count + n];
						
						if (note == 0x61) dat->note = 121;
						else if (note == 0) dat->note = 0;
						else dat->note = note + 12 * 1;
						
						dat->instrument       = instr;
						dat->volume_command   = vol;
						dat->effect_byte      = eff;
						dat->effect_parameter = eff_param;
						
#ifdef PRINT_PATTERNS
						if ((note) != 0)
						{
							const int o = (note - 1) / 12;
							const int n = (note - 1) % 12;
							/* C, C#, D, D#, E, F, F#, G, G, A, A#, B */
							printf("%c%c%X ",
								"CCDDEFFGGAAB"[n],
								"-#-#--#-#-#-"[n], o);
						}
						else printf("--- ");
						printf("%02X %02X %X%02X\t", instr, vol, eff, eff_param);
#endif
					}
#ifdef PRINT_PATTERNS
					printf("\n");
#endif
				}
			}
			
			/* seek to end of block */
			if (fseek(fp, last_pos + pattern_header.header_size + pattern_header.data_size, SEEK_SET) < 0)
				return NULL;
		}
	}

	/* load instruments */
	{
		struct pimp_instrument *instruments = NULL;
		
		mod->instrument_count = xm_header.instruments;
		/* allocate instruments */
		instruments = malloc(sizeof(struct pimp_instrument) * mod->instrument_count);
		if (NULL == instruments) return NULL;
			
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
	
	return mod;
}
