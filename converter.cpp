#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage()
{
	printf("usage: converter filenames\n");
	exit(1);
}

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
} xm_header_t;

typedef struct
{
	unsigned int   header_size;
	unsigned char  packing_type;
	unsigned short rows;
	unsigned short data_size;
} xm_pattern_header_t;

typedef struct
{
	// part 1
	unsigned int   header_size;
	char           name[22 + 1];
	unsigned char  type;
	unsigned short samples;
	
	// part 2
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
} xm_instrument_header_t;

typedef struct
{
	unsigned int  length;
	unsigned int  loop_start;
	unsigned int  loop_length;
	unsigned char volume;
	signed char   finetune;
	unsigned char type;
	unsigned char pan;
	signed char   rel_note;
	char          name[22 + 1];
} xm_sample_header_t;

bool load_xm(FILE *fp)
{
	char temp[17];
	
	fread(temp, 17, 1, fp);
	if (memcmp(temp, "Extended Module: ", 17) != 0) return false;
	printf("yay!\n");
	
	// seek to start of song-header
	fseek(fp, 60, SEEK_SET);
	
	xm_header_t xm_header;
	memset(&xm_header, 0, sizeof(xm_header));
	
	// load song-header
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
	
	printf("header size: %u\n", xm_header.header_size);
	printf("len:         %u\n", xm_header.len);
	printf("restart pos: %u\n", xm_header.restart_pos);
	printf("channels:    %u\n", xm_header.channels);
	printf("patterns:    %u\n", xm_header.patterns);
	printf("instruments: %u\n", xm_header.instruments);
	printf("flags:       %u\n", xm_header.flags);
	printf("tempo:       %u\n", xm_header.tempo);
	printf("bpm:         %u\n", xm_header.bpm);
	
	if ((xm_header.flags & 1) == 0) printf("AAAAMIIIIIGAH!\n");
	else printf("LINEAR!!\n");

	printf("dumping pattern-data...");

	// seek to start of pattern
	fseek(fp, 60 + xm_header.header_size, SEEK_SET);
	
	// load patterns
	for (unsigned p = 0; p < xm_header.patterns; ++p)
	{
		unsigned last_pos = ftell(fp);
		xm_pattern_header_t pattern_header;
		memset(&pattern_header, 0, sizeof(pattern_header));

		// load pattern-header
		fread(&pattern_header.header_size,  4, 1, fp);
		fread(&pattern_header.packing_type, 1, 1, fp);
		fread(&pattern_header.rows,         2, 1, fp);
		fread(&pattern_header.data_size,    2, 1, fp);
#ifdef PRINT_PATTERNS
		printf("pattern:              %u\n", p);
		printf("pattern-header size:  %u\n", pattern_header.header_size);
		printf("pattern packing type: %u\n", pattern_header.packing_type);
		printf("pattern rows:         %u\n", pattern_header.rows);
		printf("pattern data size:    %u\n", pattern_header.data_size);
#endif

		// seek to start of pattern data
		fseek(fp, last_pos + pattern_header.header_size, SEEK_SET);

		// load pattern data
		for (unsigned r = 0; r < pattern_header.rows; ++r)
		{
#ifdef PRINT_PATTERNS
			printf("%02X - ", r);
#endif
			for (unsigned n = 0; n < xm_header.channels; ++n)
			{
				unsigned char
					note      = 0,
					instr     = 0,
					vol       = 0,
					eff       = 0,
					eff_param = 0;
				
				fread(&note, 1, 1, fp);
				
				unsigned char pack = 0x1E;
				if (note & (1 << 7))
				{
					pack = note;
					note = 0;
				}
				
				note &= (1 << 7) - 1; // we never need the top-bit.
				
				if (pack & (1 << 0)) fread(&note,      1, 1, fp);
				if (pack & (1 << 1)) fread(&instr,     1, 1, fp);
				if (pack & (1 << 2)) fread(&vol,       1, 1, fp);
				if (pack & (1 << 3)) fread(&eff,       1, 1, fp);
				if (pack & (1 << 4)) fread(&eff_param, 1, 1, fp);

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

		// seek to end of block
		fseek(fp, last_pos + pattern_header.header_size + pattern_header.data_size, SEEK_SET);
	}
	printf("done!\n");

	printf("dumping instrument-data...");
	// load instruments
	for (unsigned i = 0; i < xm_header.instruments; ++i)
	{
		unsigned last_pos = ftell(fp);
		
		xm_instrument_header_t instrument_header;
		memset(&instrument_header, 0, sizeof(instrument_header));
		
//		printf("instrument: %i\n", i);
		fread(&instrument_header.header_size,  4,  1, fp);
		fread(&instrument_header.name,         1, 22, fp);
		fread(&instrument_header.type,         1,  1, fp);
		fread(&instrument_header.samples,      2,  1, fp);
		instrument_header.name[22] = '\0';
		
//		printf("size:    %i\n",     instrument_header.header_size);
//		printf("name:    \"%s\"\n", instrument_header.name);
//		printf("samples: %i\n",     instrument_header.samples);
		
		if (instrument_header.samples == 0)
		{
			fseek(fp, last_pos + instrument_header.header_size, SEEK_SET);
			continue;
		}
		
		unsigned last_pos2 = ftell(fp);
		
		fread(&instrument_header.sample_header_size, 4,  1, fp);
		fread(&instrument_header.sample_number,      1, 96, fp);
		fread(&instrument_header.vol_env,            2, 24, fp);
		fread(&instrument_header.pan_env,            2, 24, fp);
		fread(&instrument_header.vol_env_points,     1,  1, fp);
		fread(&instrument_header.pan_env_points,     1,  1, fp);
		fread(&instrument_header.vol_sustain,        1,  1, fp);
		fread(&instrument_header.vol_loop_start,     1,  1, fp);
		fread(&instrument_header.vol_loop_end,       1,  1, fp);
		fread(&instrument_header.pan_sustain,        1,  1, fp);
		fread(&instrument_header.pan_loop_start,     1,  1, fp);
		fread(&instrument_header.pan_loop_end,       1,  1, fp);
		fread(&instrument_header.vol_type,           1,  1, fp);
		fread(&instrument_header.pan_type,           1,  1, fp);
		fread(&instrument_header.vibrato_type,       1,  1, fp);
		fread(&instrument_header.vibrato_sweep,      1,  1, fp);
		fread(&instrument_header.vibrato_depth,      1,  1, fp);
		fread(&instrument_header.vibrato_rate,       1,  1, fp);
		fread(&instrument_header.volume_fadeout,     2,  1, fp);
		
		// seek to the end of the header (potentially variable amount of reserved bits at the end?)
		fseek(fp, instrument_header.header_size - (ftell(fp) - last_pos), SEEK_CUR);
		
//		printf("sample header size: %i\n", instrument_header.sample_header_size);
		
#if 1
		for (unsigned s = 0; s < instrument_header.samples; ++s)
		{
			xm_sample_header_t sample_header;
			memset(&sample_header, 0, sizeof(sample_header));
			
			// load sample-header
			fread(&sample_header.length,      4,  1, fp);
			fread(&sample_header.loop_start,  4,  1, fp);
			fread(&sample_header.loop_length, 4,  1, fp);
			fread(&sample_header.volume,      1,  1, fp);
			fread(&sample_header.finetune,    1,  1, fp);
			fread(&sample_header.type,        1,  1, fp);
			fread(&sample_header.pan,         1,  1, fp);
			fread(&sample_header.rel_note,    1,  1, fp);
			fseek(fp, 1, SEEK_CUR);
			fread(&sample_header.name,        1, 22, fp);
			sample_header.name[22] = '\0';
			
//			printf("length: %i\n",     sample_header.length);
//			printf("name:   \"%s\"\n", sample_header.name);
			
			// fseek(fp, sample_header.length, SEEK_CUR);
			char temp[256];
			sprintf(temp, "sample_%i_%i.raw", i, s);
			FILE *fp2 = fopen(temp, "wb");
			signed char samp = 0;
			for (unsigned i = 0; i < sample_header.length; ++i)
			{
				unsigned char data;
				fread(&data, 1, 1, fp);
				samp += data;
//				fwrite(&samp, 1, 1, fp2);
				unsigned char fjall = int(samp) + 128;
				fwrite(&fjall, 1, 1, fp2);
			}
			fclose(fp2);
		}
#endif
	}
	printf("done!\n");
	
	return true;
}

int main(int argc, char *argv[])
{
	if (argc < 2) print_usage();
	
	for (int i = 1; i < argc; ++i)
	{
		printf("loading module: %s\n", argv[i]);
		FILE *fp = fopen(argv[i], "rb");
		if (!fp) print_usage();
		
		if (!load_xm(fp))
		{
			printf("failed to load!\n");
		}
		
		fclose(fp);
	}
}
