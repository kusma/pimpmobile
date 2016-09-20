/* pimpconv.c -- Main entry-point for the tool to load and export bin-modules
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "../src/load_module.h"
#include "serialize_module.h"
#include "serializer.h"

#include "../src/pimp_module.h"
#include "../src/pimp_sample_bank.h"

void print_usage()
{
	fprintf(stderr, "Usage: pimpconv [-d] filenames\n");
	exit(EXIT_FAILURE);
}

static struct pimp_module *load_module(const char *filename, struct pimp_sample_bank *sample_bank)
{
	struct pimp_module *mod;
	
	/* open file */
	FILE *fp = fopen(filename, "rb");
	if (!fp)
	{
		perror(filename);
		exit(EXIT_FAILURE);
	}
	
	/* try to load */
	mod = load_module_xm(fp, sample_bank);
	if (NULL == mod) mod = load_module_mod(fp, sample_bank);
		
	/* report error if any */
	if (NULL == mod)
	{
		if (errno) fprintf(stderr, "%s: Failed to load module, %s\n", filename, strerror(errno));
		else       fprintf(stderr, "%s: Failed to load module\n", filename);
		exit(EXIT_FAILURE);
	}
	
	/* close file */
	fclose(fp);
	fp = NULL;
	
	/* get back on track */
	return mod;
}

static void dump_module(struct pimp_module *mod, const char *filename)
{
	FILE *fp = NULL;
	struct serializer s;
	
	ASSERT(NULL != mod);
	
	/* open target file */
	fp = fopen(filename, "wb");
	if (NULL == fp)
	{
		perror(filename);
		exit(EXIT_FAILURE);
	}
	
	/* serialize module to memory dump */
	serializer_init(&s);
	serialize_module(&s, mod);
	serializer_fixup_pointers(&s);
	
	fwrite(s.data, 1, s.pos, fp);
	
	/* close target file */
	fclose(fp);
	fp = NULL;
	
	/* get rid of serializer */
	serializer_deinit(&s);
}

/* merge the samples used in module mod located in src with the sample data in dst to reduce memory usage */
static void merge_samples(struct pimp_sample_bank *dst, const struct pimp_sample_bank *src, struct pimp_module *mod)
{
	int i;
	for (i = 0; i < mod->instrument_count; ++i)
	{
		int j;
		struct pimp_instrument *instr = pimp_module_get_instrument(mod, i);
		for (j = 0; j < instr->sample_count; ++j)
		{
			int pos;
			void *data = NULL;
			struct pimp_sample *samp = pimp_instrument_get_sample(instr, j);
			ASSERT(NULL != samp);
			
			data = pimp_sample_bank_get_sample_data(src, samp->data_ptr);
			ASSERT(NULL != data);
			
			pos = pimp_sample_bank_find_sample_data(dst, data, samp->length);
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
	FILE *fp = NULL;
	int i;
	int modules_loaded;
	int dither = 0;
	const char *sample_bank_fn = "sample_bank.bin";
	
	struct pimp_sample_bank master_sample_bank;
	struct serializer s;
	
	pimp_sample_bank_init(&master_sample_bank);
	serializer_init(&s);
	
	modules_loaded = 0;
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
			if (isatty(STDOUT_FILENO)) printf("loading %s...\n", ifn);
			mod = load_module(ifn, &sample_bank);
			
			if (NULL != mod)
			{
				char ofn[256];
				
				modules_loaded++;
				
				/* generate output filename */
				strncpy(ofn, ifn, 256);
				strncat(ofn, ".bin", 256);
				
				/* dump sample data */
				merge_samples(&master_sample_bank, &sample_bank, mod);
				
				/* dump module */
				if (isatty(STDOUT_FILENO)) printf("dumping %s...\n", ofn);
				dump_module(mod, ofn);
			}
		}
	}
	
	if (0 == modules_loaded)
	{
		fprintf(stderr, "%s: No input files\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	if (isatty(STDOUT_FILENO)) printf("dumping %s\n", sample_bank_fn);
	fp = fopen(sample_bank_fn, "wb");
	if (NULL == fp)
	{
		perror(sample_bank_fn);
		exit(EXIT_FAILURE);
	}
	
	fwrite(master_sample_bank.data, 1, master_sample_bank.size, fp);
	fclose(fp);
	fp = NULL;
	
	serializer_deinit(&s);
	
	return 0;
}
