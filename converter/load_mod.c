/* converter_mod.cpp -- Protracker MOD converting code
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "convert_sample.h"
#include "../src/pimp_module.h"
#include "../src/pimp_mixer.h" /* for pimp_loop_type enum */

int return_nearest_note(int p)
{
	double log2p, note;
	int note_int;
	
	if (p < 14) return 0; // this is not a note

	log2p = log(p) / log(2);
	note = (log2(428 << 5) - log2p) * 12.0 + 1;
	
	note_int = (int)floor(note + 0.5);

	if (note_int <= 0 || note_int > 120) return 0; // this is not a note
	return note_int;
}

static BOOL load_instrument(FILE *fp, pimp_instrument *instr)
{
	unsigned char buf[256];
	pimp_sample *samp;
		
	fread(buf, 1, 22, fp);
	buf[22] = '\0';
//	printf("sample-name: '%s' ", buf);
	
	
	// fill out the instrument-data. this is not stored in the module at all.
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
/*	samp->sustain_loop_type  = LOOP_NONE;
	samp->sustain_loop_start = 0;
	samp->sustain_loop_end   = 0; */
	samp->pan = 0;
/*	samp->ignore_default_pan_position = true; */
	samp->rel_note = 0;
	samp->vibrato_speed = 0;
	samp->vibrato_depth = 0;
	samp->vibrato_sweep = 0;
	samp->vibrato_waveform = SAMPLE_VIBRATO_SINE; /* just a dummy */
	
	fread(buf, 1, 2, fp);
	samp->length = ((buf[0] << 8) | buf[1]) << 1;
	
	fread(buf, 1, 1, fp);
	if (buf[0] > 7) samp->fine_tune = (buf[0] - 16) << 4;
	else samp->fine_tune = buf[0] << 4;
	
	fread(&samp->volume, 1, 1, fp);
	
	fread(buf, 1, 2, fp);
	samp->loop_start = ((buf[0] << 8) | buf[1]) << 1;
	if (samp->loop_start > samp->length) samp->loop_start = 0;
	
	fread(buf, 1, 2, fp);
	samp->loop_length = ((buf[0] << 8) | buf[1]) << 1;
	if (samp->loop_start + samp->loop_length > samp->length) samp->loop_length = samp->length - samp->loop_start;
/*		
	printf("sample: %02X - ", i + 1);
	printf("loop start: %d ", samp.loop_start);
	printf("loop end: %d ", samp.loop_end);
*/	
	if ((samp->loop_start <= 2) && (samp->loop_length <= 4)) samp->loop_type = LOOP_TYPE_NONE;
	else samp->loop_type = LOOP_TYPE_FORWARD;
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

pimp_module *load_module_mod(FILE *fp)
{
	int i, p;
	pimp_module *mod;
	int channels;
	int max_pattern;
	
	u8 sig_data[4];
	u32 sig;
	
	fseek(fp, 1080, SEEK_SET);
	
	fread(&sig_data, 1, 4, fp);
	sig = MAKE_WORD(sig_data[0], sig_data[1], sig_data[2], sig_data[3]);
	channels = get_channel_count(sig);
	if (channels > 0) return NULL;
	
	mod = (pimp_module *)malloc(sizeof(pimp_module));
	if (NULL == mod) return NULL;
	
	memset(mod, 0, sizeof(pimp_module));
	
	mod->period_low_clamp           = 113; // B-3 in MOD
	mod->period_high_clamp          = 856; // C-1 in MOD
	
	/* this should be correct if no notes outside this range is used */
	mod->period_low_clamp           = 108; // B-3 with fine tune 8 in MOD
	mod->period_high_clamp          = 907; // C-1 wuth fine tune -8 in MOD
	
	mod->volume = 64;
	mod->tempo  = 6;
	mod->bpm    = 125;
	
	mod->flags = FLAG_TEMOR_EXTRA_DELAY | FLAG_TEMOR_MEMORY | FLAG_PORTA_NOTE_MEMORY;
	
	// song name
	char name[20 + 1];
	rewind(fp);
	fread(name, 20, 1, fp);
	name[20] = '\0';

	strcpy(mod->name, name);

	/* setup channel-settings */
	{
		mod->channel_count = channels;
		pimp_channel *channels = (pimp_channel *)malloc(sizeof(pimp_channel) * mod->channel_count);
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
	
	mod->instrument_count = 31;
	pimp_instrument *instruments = (pimp_instrument *)malloc(sizeof(pimp_instrument) * mod->instrument_count);
	if (NULL == instruments) return NULL;
		
	pimp_set_ptr(&mod->instrument_ptr, instruments);
	
	for (i = 0; i < mod->instrument_count; ++i)
	{
		BOOL ret = load_instrument(fp, &instruments[i]);
		if (FALSE == ret)
		{
			/* TODO: cleanup */
			return NULL;
		}
	}

	static unsigned char buf[256];

	/* order count */
	fread(buf, 1, 1, fp);
	mod->order_count = buf[0];
	
	if (mod->order_count > 128)
	{
		fprintf(stderr, "warning: excessive orders in module, discarding.\n");
		mod->order_count = 128;
	}

	/* allocate memory */
	{
		unsigned char *orders = (unsigned char *)malloc(mod->order_count);
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
	
	
	mod->pattern_count = max_pattern + 1;
	pimp_pattern *patterns = (pimp_pattern *)malloc(sizeof(pimp_pattern) * mod->pattern_count);
	if (NULL == patterns) return NULL;
	
	memset(patterns, 0, sizeof(pimp_pattern) * mod->pattern_count);
	pimp_set_ptr(&mod->pattern_ptr, patterns);

	
	/* load patterns and track the min and max note. this is used to detect if the module has notes outside traditional mod-limits */
	int min_period =  99999;
	int max_period = -99999;
	for (p = 0; p < mod->pattern_count; ++p)
	{
		pimp_pattern *pat = &patterns[p];
		pat->row_count = 64;

		pimp_pattern_entry *pattern_data = (pimp_pattern_entry *)malloc(sizeof(pimp_pattern_entry) * mod->channel_count * pat->row_count);
		if (NULL == pattern_data)
		{
			return NULL;
		}
		pimp_set_ptr(&pat->data_ptr, pattern_data);

		/* clear memory */
		memset(pattern_data, 0, sizeof(pimp_pattern_entry) * mod->channel_count * pat->row_count);
		
		for (i = 0; i < 64; ++i)
		{
			int j;
			for (j = 0; j < channels; ++j)
			{
				pimp_pattern_entry *pe = &pattern_data[i * channels + j];
				fread(buf, 1, 4, fp);
				int period = ((buf[0] & 0x0F) << 8) + buf[1];
				
				pe->instrument       = (buf[0] & 0x0F0) + (buf[2] >> 4);
				pe->note             = return_nearest_note(period); // - 12;
				pe->effect_byte      = buf[2] & 0xF;
				pe->effect_parameter = buf[3];
				
				if (period > max_period) max_period = period;
				if (period < min_period) min_period = period;
			}
		}
	}
	
	// if there are periods in the file outside the default pt2-range, assume ft2-range
	if (min_period < mod->period_low_clamp || max_period > mod->period_high_clamp)
	{
		mod->period_low_clamp = 1;
		mod->period_high_clamp = 32767;
	}

	/* load samples */
	for (i = 0; i < 31; ++i)
	{
		pimp_sample *samp;
		pimp_instrument *instr = &instruments[i];
		
		/* only one sample per instrument in MOD */
		instr->sample_count = 1;
		samp = (pimp_sample *)malloc(sizeof(pimp_sample));
		if (NULL == samp) return FALSE;
	/*	PIMP_SET_PTR(instr->sample_ptr, samp); */
		pimp_set_ptr(&instr->sample_ptr, samp);

		if (samp->length > 0 && samp->length > 2)
		{
			enum pimp_sample_format src_format = PIMP_SAMPLE_S8;
			enum pimp_sample_format dst_format = PIMP_SAMPLE_U8;
			
			void *dst_waveform;
			void *src_waveform = malloc(samp->length);

			if (NULL == src_waveform) return FALSE;
			fread(src_waveform, 1, samp->length, fp);
			
			ASSERT(_pimp_sample_format_get_size(src_format) == _pimp_sample_format_get_size(dst_format));
			
			dst_waveform = malloc(samp->length * _pimp_sample_format_get_size(dst_format));
			if (NULL == dst_waveform) return FALSE;
			
			_pimp_convert_sample(dst_waveform, dst_format, src_waveform, src_format, samp->length);
			
			/* TODO: insert into sample bank. */
			samp->data_ptr = 0;
			free(src_waveform);
		}
		else
		{
			/* no sample (WHAT TO DOO?!) */
			samp->data_ptr = 0;
		}
	}
	
	
#if 0	
	/* check file length */
	int eof1 = feof(fp);
	fread(buf, 1, 1, fp);	
	int eof2 = feof(fp);
	if ((eof1 != 0) == (eof2 != 0))
	{
		/* uhm, still need to free the rest later. */
		free(mod);
		mod = NULL;
		
		return NULL;
	}
#endif
	
	return mod;
}
