#include "../../src/pimp_mod_context.h"
#include "../../src/pimp_sample_bank.h"
#include "../../src/pimp_render.h"
#include "../../converter/load_module.h"

#include <stdio.h>
#include <stdlib.h>

#define SAMPLES 100000

int main(int argc, char *argv[])
{
	int i;
	pimp_mixer mixer;
	pimp_mod_context ctx;
	struct pimp_sample_bank sample_bank;

	s32 mixbuf[SAMPLES];
	signed char buf[SAMPLES];
	
	FILE *fp;
	char *ifn, *ofn;
	const pimp_module *mod;
	
	/* check parameters */
	if (argc < 3)
	{
		puts("too few arguments");
		exit(1);
	}
	
	ifn = argv[1];
	ofn = argv[2];

	/* open input file */
	fp = fopen(ifn, "rb");
	if (NULL == fp)
	{
		fprintf(stderr, "*** failed to open %s for reading\n", ifn);
		exit(1);
	}
	
	/* load module */
	pimp_sample_bank_init(&sample_bank);
	mod = load_module_xm(fp, &sample_bank);
	
	/* close input file */
	fclose(fp);
	fp = NULL;
	
	/* check if module got loaded */
	if (NULL == mod)
	{
		fprintf(stderr, "*** failed to load module %s\n", ifn);
		exit(1);
	}

	/* setup rendering */
	mixer.mix_buffer = mixbuf;
	pimp_mod_context_init(&ctx, mod, (const u8*)sample_bank.data, &mixer);
	
	/* render module to buffer */
	pimp_render(&ctx, buf, SAMPLES);

	/* open output file for writing */
	fp = fopen(ofn, "wb");
	if (NULL == fp)
	{
		fprintf(stderr, "*** failed to open %s for writing\n", ofn);
		exit(1);
	}

	/* write buf to file */
	fwrite(buf, 1, SAMPLES, fp);
	
	/* close file */
	fclose(fp);
	fp = NULL;

	return 0;
}
