#include <stdio.h>
#include <assert.h>

#include "load_module.h"
#include "serialize_module.h"
#include "serializer.h"

#include "../src/pimp_module.h"
#include "../src/pimp_sample_bank.h"

void print_usage()
{
	fprintf(stderr, "usage: converter filenames\n");
	exit(1);
}

static struct pimp_module *load_module(const char *filename, struct pimp_sample_bank *sample_bank)
{
	struct pimp_module *mod;
	
	/* open file */
	FILE *fp = fopen(filename, "rb");
	if (!fp)
	{
		printf("file not found!\n");
		return NULL;
	}
	
	/* try to load */
	mod = load_module_xm(fp, sample_bank);
	if (NULL == mod) mod = load_module_mod(fp, sample_bank);
	if (NULL == mod) fprintf(stderr, "failed to load module!\n");
	
	/* close file */
	fclose(fp);
	fp = NULL;
	
	return mod;
}

static void dump_module(struct pimp_module *mod, const char *filename)
{
	struct serializer s;
	
	ASSERT(NULL != mod);
	
	serializer_init(&s);
	serialize_module(&s, mod);
	serializer_fixup_pointers(&s);

	FILE *fp = fopen(filename, "wb");
	if (NULL == fp)
	{
		fprintf(stderr, "failed to open output file\n");
		return;
	}
	
	fwrite(s.data, 1, s.pos, fp);
	fclose(fp);
	fp = NULL;
	
	
	serializer_deinit(&s);
}

/* merge the samples used in module mod located in src with the sample data in dst to reduce memory usage */
static void merge_samples(struct pimp_sample_bank *dst, const struct pimp_sample_bank *src, struct pimp_module *mod)
{
	int i;
	for (i = 0; i < mod->instrument_count; ++i)
	{
		int j;
		pimp_instrument *instr = pimp_module_get_instrument(mod, i);
		for (j = 0; j < instr->sample_count; ++j)
		{
			pimp_sample *samp = pimp_instrument_get_sample(instr, j);
			ASSERT(NULL != samp);
			
			void *data = pimp_sample_bank_get_sample_data(src, samp->data_ptr);
			ASSERT(NULL != data);
			
			int pos = pimp_sample_bank_find_sample_data(dst, data, samp->length);
			if (pos < 0)
			{
				pos = pimp_sample_bank_insert_sample_data(dst, data, samp->length);
			}
			samp->data_ptr = pos;
		}
	}
}

int main(int argc, char *argv[])
{
	int i;
	int dither = 0;

	struct serializer s;
	serializer_init(&s);
	
	struct pimp_sample_bank master_sample_bank;
	pimp_sample_bank_init(&master_sample_bank);
	
	for (i = 1; i < argc; ++i)
	{
		const char *arg = argv[i];
		if (arg[0] == '-')
		{
			if (strlen(arg) < 2) print_usage();
			
			switch (arg[1])
			{
				case 'd':
				case 'D':
					dither = 1;
				break;
				default: print_usage();
			}
		}
		else
		{
			struct pimp_module *mod;
			const char *ifn = arg;
			
			struct pimp_sample_bank sample_bank;
			pimp_sample_bank_init(&sample_bank);
			
			/* load module */
			printf("loading %s...\n", ifn);
			mod = load_module(ifn, &sample_bank);
			
			if (NULL != mod)
			{	
				/* generate output filename */
				char ofn[256];
				strncpy(ofn, ifn, 256);
				strncat(ofn, ".bin", 256);
				
				/* dump sample data */
				merge_samples(&master_sample_bank, &sample_bank, mod);

				/* dump module */
				printf("dumping %s...\n", ofn);
				dump_module(mod, ofn);
				
				/* cleanup */
				free(ofn);
			}
		}
	}
	
	printf("dumping sample_bank.bin\n");
	FILE *fp = fopen("sample_bank.bin", "wb");
	if (NULL == fp)
	{
		fprintf(stderr, "failed to open output file\n");
		return 1;
	}
	
	fwrite(master_sample_bank.data, 1, master_sample_bank.size, fp);
	fclose(fp);
	fp = NULL;
	
	serializer_deinit(&s);

	return 0;
}
