#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "converter.h"

module_t *load_module_s3m(FILE *fp)
{
	
	/* check file-signature */
	char sig[4];
	fseek(fp, 0x2C, SEEK_SET);
	fread(sig, 1, 4, fp);
	if (memcmp(sig, "SCRM", 4) != 0) return NULL;
	
	/* check a second signature. */
	unsigned char sig2;
	fseek(fp, 0x1C, SEEK_SET);
	fread(&sig2, 1, 1, fp);
	if (sig2 != 0x1A) return NULL;
	
	/* check that the type is correct. just another sanity-check. */
	unsigned char type;
	fread(&type, 1, 1, fp);
	if (type != 16) return NULL;
	
	printf("huston, we have s3m.\n");
	
	module_t *mod = (module_t *)malloc(sizeof(module_t));
	if (mod == 0)
	{
		printf("out of memory\n");
		fclose(fp);
		exit(1);		
	}
	memset(mod, 0, sizeof(module_t));
	
	/* veryfy these */
	mod->instrument_vibrato_use_linear_frequency_table = false;
	mod->volume_slide_in_tick0        = false;
	mod->vibrato_in_tick0             = false;
	mod->tremor_extra_delay           = true;
	mod->tremor_has_memory            = true;
	mod->retrig_kills_note            = false;
	mod->note_cut_kills_note          = false;
	mod->allow_nested_loops           = true;
	mod->retrig_note_source_is_period = false;
	mod->delay_global_volume          = false;
	mod->sample_offset_clamp          = false;
	mod->tone_porta_share_memory      = false; // unsure, need to check
	mod->remember_tone_porta_target   = true;
	
	/* load module-name */
	char name[28 + 1];
	fseek(fp, 0x0, SEEK_SET);
	fread(&name, 1, 28, fp);
	name[28] = '\0';
	strcpy(mod->name, name);
	
	printf("it's called \"%s\"\n", mod->name);
	
	unsigned short ordnum, insnum, patnum, flags;
	fseek(fp, 0x20, SEEK_SET);
	fread(&ordnum, 1, 2, fp);
	fread(&insnum, 1, 2, fp);
	fread(&patnum, 1, 2, fp);
	fread(&flags,  1, 2, fp);
	printf("orders: %d\ninstruments: %d\npatterns: %d\nflags: %x\n", ordnum, insnum, patnum, flags);
	fseek(fp, 0x1, SEEK_CUR); /* skip tracker version */
	
	mod->play_order_length = ordnum;
	mod->pattern_count     = patnum;
	mod->instrument_count  = insnum;
	
	mod->use_linear_frequency_table = (flags & 4) == 0;
	mod->vol0_optimizations         = (flags & 8) != 0;
	
	if (flags & 16) // amiga-limits
	{
		printf("AMIGAH limits.\n");
		mod->period_low_clamp  = (flags & 4) == 0 ? 0     : 113; // linear period is wrong (not the amiga-limit, but rather the xm-limit)
		mod->period_high_clamp = (flags & 4) == 0 ? 10752 : 856; // linear period is wrong (not the amiga-limit, but rather the xm-limit)
	}
	else // no amiga-limits
	{
		mod->period_low_clamp  = (flags & 4) == 0 ? 0     : 1;     // fist value is linear, second is amiga
		mod->period_high_clamp = (flags & 4) == 0 ? 10752 : 32767; // fist value is linear, second is amiga
	}

//	mod->num_channels = ;                 /* number of channels in the pattern-data of the module. */
/*
	u8  play_order_repeat_position;
	u8  *play_order;
*/
	
	mod->initial_global_volume        = 64;
	mod->initial_tempo                = 6;
	mod->initial_bpm                  = 125;

	
	/* todo: load more */
	
	return mod;
}
