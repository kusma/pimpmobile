/* load_xm.c -- FastTracker XM loading code
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

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
	unsigned int   header_size;
	unsigned short len;
	unsigned short restart_pos;
	unsigned short channels;
	unsigned short patterns;
	unsigned short instruments;
	unsigned short flags;
	unsigned short tempo;
	unsigned short bpm;
} xm_header;

typedef struct
{
	unsigned int   header_size;
	unsigned char  packing_type;
	unsigned short rows;
	unsigned short data_size;
} xm_pattern_header;

typedef struct
{
	/* part 1 */
	unsigned int   header_size;
	char           name[22 + 1];
	unsigned char  type;
	unsigned short samples;
	
	/* part 2 */
	unsigned int   sample_header_size;
	unsigned char  sample_number[96];
	unsigned short vol_env[48];
	unsigned short pan_env[48];
	unsigned char  vol_env_points;
	unsigned char  pan_env_points;
	unsigned char  vol_sustain;
	unsigned char  vol_loop_start;
	unsigned char  vol_loop_end;
	unsigned char  pan_sustain;
	unsigned char  pan_loop_start;
	unsigned char  pan_loop_end;
	unsigned char  vol_type;
	unsigned char  pan_type;
	unsigned char  vibrato_type;
	unsigned char  vibrato_sweep;
	unsigned char  vibrato_depth;
	unsigned char  vibrato_rate;
	unsigned short volume_fadeout;
} xm_instrument_header;

typedef struct
{
	unsigned int  length;
	unsigned int  loop_start;
	unsigned int  loop_length;
	unsigned char volume;
	signed char   fine_tune;
	unsigned char type;
	unsigned char pan;
	signed char   rel_note;
	char          name[22 + 1];
} xm_sample_header;

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
	fread(&ih.header_size,  4,  1, fp);
	fread(&ih.name,         1, 22, fp);
	fread(&ih.type,         1,  1, fp);
	fread(&ih.samples,      2,  1, fp);
	ih.name[22] = '\0';
	
	if (ih.samples != 0)
	{
		/* read part 2 */
		fread(&ih.sample_header_size, 4,  1, fp);
		fread(&ih.sample_number,      1, 96, fp);
		fread(&ih.vol_env,            2, 24, fp);
		fread(&ih.pan_env,            2, 24, fp);
		fread(&ih.vol_env_points,     1,  1, fp);
		fread(&ih.pan_env_points,     1,  1, fp);
		fread(&ih.vol_sustain,        1,  1, fp);
		fread(&ih.vol_loop_start,     1,  1, fp);
		fread(&ih.vol_loop_end,       1,  1, fp);
		fread(&ih.pan_sustain,        1,  1, fp);
		fread(&ih.pan_loop_start,     1,  1, fp);
		fread(&ih.pan_loop_end,       1,  1, fp);
		fread(&ih.vol_type,           1,  1, fp);
		fread(&ih.pan_type,           1,  1, fp);
		fread(&ih.vibrato_type,       1,  1, fp);
		fread(&ih.vibrato_sweep,      1,  1, fp);
		fread(&ih.vibrato_depth,      1,  1, fp);
		fread(&ih.vibrato_rate,       1,  1, fp);
		fread(&ih.volume_fadeout,     2,  1, fp);
	}
	
	pimp_set_ptr(&instr->vol_env_ptr, NULL);
	if ((ih.vol_type & 1) == 1)
	{
		struct pimp_envelope *env = (struct pimp_envelope*)malloc(sizeof(struct pimp_envelope));
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
		struct pimp_envelope *env = (struct pimp_envelope*)malloc(sizeof(struct pimp_envelope));
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
			samples = (struct pimp_sample *)malloc(sizeof(struct pimp_sample) * instr->sample_count);
			if (NULL == samples) return FALSE;
		}
		pimp_set_ptr(&instr->sample_ptr, samples);
	}
	
	fseek(fp, ih.header_size - (ftell(fp) - last_pos), SEEK_CUR);
	
	/* sample headers allocated dynamically because data needs to be remembered. we're doing two passes here - one for sample headers, and one for sample data. */
	sample_headers = (xm_sample_header *)malloc(sizeof(xm_sample_header) * ih.samples);
	if (NULL == sample_headers) return FALSE;
		
	memset(sample_headers, 0, sizeof(xm_sample_header) * ih.samples);
	
	/* all sample headers are stored first, and THEN comes all sample-data. now it's time for them headers */
	for (s = 0; s < instr->sample_count; ++s)
	{
		struct pimp_sample *samp = NULL;
		xm_sample_header *sh = &sample_headers[s];
		
		/* load sample-header */
		fread(&sh->length,      4,  1, fp);
		fread(&sh->loop_start,  4,  1, fp);
		fread(&sh->loop_length, 4,  1, fp);
		fread(&sh->volume,      1,  1, fp);
		fread(&sh->fine_tune,    1,  1, fp);
		fread(&sh->type,        1,  1, fp);
		fread(&sh->pan,         1,  1, fp);
		fread(&sh->rel_note,    1,  1, fp);
		fseek(fp, 1, SEEK_CUR);
		fread(&sh->name,        1, 22, fp);
		sh->name[22] = '\0';
		
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
				signed short data = 0;
				fread(&data, 1, 2, fp);
				prev += data;
				((signed short*)src_waveform)[i] = prev;
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
				signed char data = 0;
				fread(&data, 1, 1, fp);
				prev += data;
				((signed char*)src_waveform)[i] = prev;
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
		char temp[17];
		rewind(fp);
		fread(temp, 17, 1, fp);
		if (memcmp(temp, "Extended Module: ", 17) != 0) return NULL;
	}
	
	/* read song name */
	fread(name, 20, 1, fp);
	name[20] = '\0';

	/* just another file-sanity-check */
	{
		unsigned char magic;
		fread(&magic, 1, 1, fp);
		if (magic != 0x1a) return NULL;
	}
	
	/* read tracker name (seems to not be very reliable for bug-emulating) */
	fread(tracker_name, 20, 1, fp);
	tracker_name[20] = '\0';
	
	/* check version */
	{
		unsigned short version;
		fread(&version, 2, 1, fp);
		if (version < 0x104)
		{
			fprintf(stderr, "too old file format version, please open and resave in ft2\n");
			return NULL;
		}
	}
	
	mod = (pimp_module *)malloc(sizeof(pimp_module));
	if (NULL == mod) return NULL;
	
	memset(mod, 0, sizeof(pimp_module));

	strcpy(mod->name, name);
	
	memset(&xm_header, 0, sizeof(xm_header));
	
	/* load song-header */
	fread(&xm_header.header_size, 4, 1, fp);
	fread(&xm_header.len,         2, 1, fp);
	fread(&xm_header.restart_pos, 2, 1, fp);
	fread(&xm_header.channels,    2, 1, fp);
	fread(&xm_header.patterns,    2, 1, fp);
	fread(&xm_header.instruments, 2, 1, fp);
	fread(&xm_header.flags,       2, 1, fp);
	fread(&xm_header.tempo,       2, 1, fp);
 	fread(&xm_header.bpm,         2, 1, fp);
	
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
		
		fread(orders, sizeof(u8), mod->order_count, fp);
		pimp_set_ptr(&mod->order_ptr, orders);
	}
	mod->order_repeat = xm_header.restart_pos;
	
	{
		struct pimp_channel *channels;
		mod->channel_count = xm_header.channels;
		channels = (struct pimp_channel *)malloc(sizeof(struct pimp_channel) * mod->channel_count);
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
	fseek(fp, 60 + xm_header.header_size, SEEK_SET);
	
	{
		int p;
		struct pimp_pattern *patterns;
		
		mod->pattern_count = xm_header.patterns;
		/* allocate memory for patterns */
		patterns = (struct pimp_pattern *)malloc(sizeof(struct pimp_pattern) * mod->pattern_count);
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
			fread(&pattern_header.header_size,  4, 1, fp);
			fread(&pattern_header.packing_type, 1, 1, fp);
			fread(&pattern_header.rows,         2, 1, fp);
			fread(&pattern_header.data_size,    2, 1, fp);
			
			pat = &patterns[p];
			pat->row_count = pattern_header.rows;
			
			/* allocate memory for pattern data */
			pattern_data = (struct pimp_pattern_entry *)malloc(sizeof(struct pimp_pattern_entry) * mod->channel_count * pat->row_count);
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
			fseek(fp, last_pos + pattern_header.header_size, SEEK_SET);
			
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
						unsigned char pack;
						unsigned char
							note      = 0,
							instr     = 0,
							vol       = 0,
							eff       = 0,
							eff_param = 0;
						
						fread(&note, 1, 1, fp);
						
						pack = 0x1E;
						if (note & (1 << 7))
						{
							pack = note;
							note = 0;
						}
						
						note &= (1 << 7) - 1; /* we never need the top-bit. */
						
						if (pack & (1 << 0)) fread(&note,      1, 1, fp);
						if (pack & (1 << 1)) fread(&instr,     1, 1, fp);
						if (pack & (1 << 2)) fread(&vol,       1, 1, fp);
						if (pack & (1 << 3)) fread(&eff,       1, 1, fp);
						if (pack & (1 << 4)) fread(&eff_param, 1, 1, fp);
						
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
			fseek(fp, last_pos + pattern_header.header_size + pattern_header.data_size, SEEK_SET);
		}
	}

	/* load instruments */
	{
		struct pimp_instrument *instruments = NULL;
		
		mod->instrument_count = xm_header.instruments;
		/* allocate instruments */
		instruments = (struct pimp_instrument *)malloc(sizeof(struct pimp_instrument) * mod->instrument_count);
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
