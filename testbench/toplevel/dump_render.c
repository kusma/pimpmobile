#include "../../src/pimp_mod_context.h"
#include "../../src/pimp_sample_bank.h"
#include "../../src/pimp_render.h"
#include "../../converter/load_module.h"

int main(int argc, char *argv[])
{
	pimp_mixer mixer;
	pimp_mod_context ctx;
	struct pimp_sample_bank sample_bank;
	
	const pimp_module *mod;
	
	const char *filename = "test.xm";

	FILE *fp = fopen(filename, "rb");
	if (NULL == fp)
	{
		fprintf(stderr, "*** failed to load %s\n", filename);
		exit(1);
	}
	
	pimp_sample_bank_init(&sample_bank);
	mod = load_module_xm(fp, &sample_bank);
	fclose(fp);
	fp = NULL;
	
	if (NULL == mod)
	{
		fprintf(stderr, "*** failed to load %s\n", filename);
		exit(1);
	}

#if 0
#define SAMPLES 10000

	u32 mixbuf[SAMPLES];
	mixer.mix_buffer = mixbuf;
	pimp_mod_context_init(&ctx, mod, (const u8*)sample_bank.data, &mixer);
	
	signed char buf[SAMPLES];
	pimp_render(&ctx, buf, SAMPLES);

	fp = fopen("output.bin", "wb");
	if (NULL != fp)
	{
		fwrite(buf, 1, SAMPLES, fp);
		fclose(fp);
	}
#else
	static u32 mixbuf[304];
	mixer.mix_buffer = mixbuf;
	pimp_mod_context_init(&ctx, mod, (const u8*)sample_bank.data, &mixer);
	
	fp = fopen("output.bin", "wb");
	if (NULL != fp)
	{
		int i;
		for (i = 0; i < 100; ++i)
		{
			static signed char buf[304];
			pimp_render(&ctx, buf, 304);
			fwrite(buf, 1, 304, fp);
		}
		fclose(fp);
	}
#endif

	return 0;
}
