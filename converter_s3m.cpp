#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "converter.h"

module_t *load_module_s3m(FILE *fp)
{
	fseek(fp, 0x2C, SEEK_SET);
	
	char sig[4];
	fread(sig, 1, 4, fp);
	if (memcmp(sig, "SCRM", 4) != 0) return NULL;
	
	printf("huston, we have s3m.\n");
	
	module_t *mod = (module_t *)malloc(sizeof(module_t));
	if (mod == 0)
	{
		printf("out of memory\n");
		fclose(fp);
		exit(1);		
	}
	memset(mod, 0, sizeof(module_t));
	
	
	/* todo: load */
	
	return mod;
}
