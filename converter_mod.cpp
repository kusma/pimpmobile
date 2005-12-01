#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "converter.h"

int return_nearest_note(int p)
{
	if( p < 14 ) return 0; // this is not a note

	double log2p = log(p)/log(2);
	double note = (log2( 428 << 5) - log2p) * 12.0 + 1;
	
	int note_int = floor( note + .5 );

	if (note_int <= 0 || note_int > 120) return 0; // this is not a note
	return note_int;
}

module_t *load_module_MOD(FILE *fp)
{
	fseek(fp, 1080, SEEK_SET);

	unsigned channels;
	
	unsigned sig;
	fread(&sig, 1, 4, fp);
	switch (sig) /* riprap from open cubic player */
	{
		case 0x4E484331: channels = 1; break; // 1CHN
		case 0x4E484332: channels = 2; break; // 2CHN
		case 0x4E484333: channels = 3; break; // 3CHN
		
		case 0x2E4B2E4D: // M.K.
		case 0x214B214D: // M!K!
		case 0x34544C46: // FLT4
		case 0x4E484334: // 4CHN
			channels = 4;
		break;
		
		case 0x4E484335: channels = 5; break; // 5CHN
		case 0x4E484336: channels = 6; break; // 6CHN
		case 0x4E484337: channels = 7; break; // 7CHN
		
		case 0x31384443: // CD81
		case 0x4E484338: // 8CHN
			channels = 8;
		break;
		
		case 0x4E484339: channels =  9; break; // 9CHN
		case 0x48433031: channels = 10; break; // 10CH
		case 0x48433131: channels = 11; break; // 11CH
		case 0x48433231: channels = 12; break; // 12CH
		case 0x48433331: channels = 13; break; // 13CH
		case 0x48433431: channels = 14; break; // 14CH
		case 0x48433531: channels = 15; break; // 15CH
		case 0x48433631: channels = 16; break; // 16CH
		case 0x48433731: channels = 17; break; // 17CH
		case 0x48433831: channels = 18; break; // 18CH
		case 0x48433931: channels = 19; break; // 19CH
		case 0x48433032: channels = 20; break; // 20CH
		case 0x48433132: channels = 21; break; // 21CH
		case 0x48433232: channels = 22; break; // 22CH
		case 0x48433332: channels = 23; break; // 23CH
		case 0x48433432: channels = 24; break; // 24CH
		case 0x48433532: channels = 25; break; // 25CH
		case 0x48433632: channels = 26; break; // 26CH
		case 0x48433732: channels = 27; break; // 27CH
		case 0x48433832: channels = 28; break; // 28CH
		case 0x48433932: channels = 29; break; // 29CH
		case 0x48433033: channels = 30; break; // 30CH
		case 0x48433133: channels = 31; break; // 31CH
		case 0x48433233: channels = 32; break; // 32CH
		
		default: return NULL; // not a mod as far as we can see.
	}
	printf("chans: %i\n", channels);
	
	module_t *mod = (module_t *)malloc(sizeof(module_t));
	if (mod == 0)
	{
		printf("out of memory\n");
		fclose(fp);
		exit(1);		
	}
	memset(mod, 0, sizeof(module_t));
	
	
	// song name
	char name[20 + 1];
	rewind(fp);
	fread(name, 20, 1, fp);
	name[20] = '\0';
	printf("name : \"%s\"\n", name);

	strcpy(mod->name, name);
	mod->num_channels = channels;
	mod->instruments = (instrument_t*) malloc(sizeof(instrument_t) * 31);
	mod->instrument_count = 31;
	assert(mod->instruments != 0);
	memset(mod->instruments, 0, sizeof(instrument_t) * 31);
	
	static unsigned char buf[256];
	for (int i = 0; i < 31; ++i)
	{
		instrument_t &instr = mod->instruments[i];
		
		instr.sample_headers = (sample_header_t*)malloc(sizeof(sample_header_t));
		assert(instr.sample_headers != NULL);
		memset(instr.sample_headers, 0, sizeof(sample_header_t));
		
		sample_header_t &samp = instr.sample_headers[0];
		
		fread(buf, 1, 22, fp);
		buf[22] = '\0';
		printf("sample-name: '%s' ", buf);
		
		
		// fill out the instrument-data. this is not stored in the module at all.
		strcpy(instr.name, (const char*)buf);
		instr.volume_envelope = NULL;
		instr.panning_envelope = NULL;
		instr.pitch_envelope  = NULL;
		instr.fadeout_rate = 0;
		instr.new_note_action        = NNA_CUT; /* "emulate" PT2-behavour on IT-fields */
		instr.duplicate_check_type   = DCT_OFF;
		instr.duplicate_check_action = DCA_CUT;
		instr.pitch_pan_separation   = 0;
		instr.pitch_pan_center       = 0;
		instr.sample_count           = 1;
		memset(instr.sample_map, 1, 120);
		
		strcpy(samp.name, (const char*)buf);
		samp.waveform = NULL;
		samp.format = SAMPLE_SIGNED_8BIT;
		samp.is_stereo = false;
		samp.sustain_loop_type  = LOOP_NONE;
		samp.sustain_loop_start = 0;
		samp.sustain_loop_end   = 0;
		samp.default_pan_position = 0;
		samp.ignore_default_pan_position = true;
		samp.sample_note_offset = 0;
		samp.note_period_translation_map_index = 0; /* do we need this ? */
		samp.vibrato_speed = 0;
		samp.vibrato_depth = 0;
		samp.vibrato_sweep = 0;
		samp.vibrato_waveform = SAMPLE_VIBRATO_SINE; /* just a dummy */
		
		fread(buf, 1, 2, fp);
		samp.length = ((buf[0] << 8) | buf[1]) << 1;
		printf("length: %i ", samp.length);
		
		fread(buf, 1, 1, fp);
		if (buf[0] > 7) samp.finetune = (buf[0] - 16) << 4;
		else samp.finetune = buf[0] << 4;
		printf("finetune: %i ", samp.finetune);
		
		fread(&samp.default_volume, 1, 1, fp);
		printf("vol: %i ", samp.default_volume);
		
		fread(buf, 1, 2, fp);
		samp.loop_start = ((buf[0] << 8) | buf[1]) << 1;
		printf("loop start: %i ", samp.loop_start);
		
		fread(buf, 1, 2, fp);
		unsigned loop_length = ((buf[0] << 8) | buf[1]) << 1;
		samp.loop_end = samp.loop_start + loop_length;
		printf("loop end: %i\n", samp.loop_end);
		
		if ((samp.loop_start == 0) && (loop_length < 4)) samp.loop_type = LOOP_NONE;
		else samp.loop_type = LOOP_FORWARD;
	}
	

	fread(buf, 1, 1, fp);
	mod->play_order_length = buf[0];
	printf("song length: %i\n", mod->play_order_length);

	mod->play_order_repeat_position = 0;
	fseek(fp, 1, SEEK_CUR); // discard unused byte (this may be repeat position, but that is impossible to tell)
	
	mod->play_order = (u8*)malloc(mod->play_order_length);

	mod->pattern_count = 0;
	for (int i = 0; i < mod->play_order_length; ++i)
	{
		fread(&mod->play_order[i], 1, 1, fp);
		if (mod->play_order[i] > mod->pattern_count) mod->pattern_count = mod->play_order[i];
	}
	mod->pattern_count++;
	fseek(fp, 128 - mod->play_order_length, SEEK_CUR); // discard unused orders
	
	fseek(fp, 4, SEEK_CUR); // discard mod-signature
	
	mod->patterns = (pattern_header_t*) malloc(sizeof(pattern_header_t) * mod->pattern_count);
	assert(mod->patterns != 0);
	memset(mod->patterns, 0, sizeof(pattern_header_t) * mod->pattern_count);
	
	for (unsigned p = 0; p < mod->pattern_count; ++p)
	{
		pattern_header_t &pat = mod->patterns[p];
		pat.pattern_data = (pattern_entry_t *)malloc(sizeof(pattern_entry_t) * channels * 64);
		assert(pat.pattern_data != NULL);
		memset(pat.pattern_data, 0, sizeof(pattern_entry_t) * channels * 64);
		
		for (unsigned i = 0; i < 64; ++i)
		{
			for (unsigned j = 0; j < channels; ++j)
			{
				pattern_entry_t &pe = pat.pattern_data[i * channels + j];
				fread(buf, 1, 4, fp);
				pe.instrument       = (buf[0] & 0x0F0) + (buf[2] >> 4);
				pe.note             = return_nearest_note(((buf[0] & 0x0F) <<8 ) + buf[1]);
				pe.effect_byte      = buf[2] & 0xF;
				pe.effect_parameter = buf[3];
				
				if (pe.note != 0)
				{
					const int o = (pe.note - 1) / 12;
					const int n = (pe.note - 1) % 12;
					/* C, C#, D, D#, E, F, F#, G, G, A, A#, B */
					printf("%c%c%X ",
						"CCDDEFFGGAAB"[n],
						"-#-#--#-#-#-"[n], o);
				}
				else printf("--- ");
				
				printf("%02X %02X %X%02X\t", pe.instrument, pe.volume_command, pe.effect_byte, pe.effect_parameter);
			}
			printf("\n");
		}
	}
	
	for (unsigned i=0; i<31; i++)
	{
		instrument_t &instr = mod->instruments[i];
		sample_header_t &samp = instr.sample_headers[0];
		
		if (samp.length > 0)
		{
			samp.waveform = malloc(samp.length);
			assert(samp.waveform != NULL);
			fread(samp.waveform, 1, samp.length, fp);
		}
	}
	
	return mod;
}
