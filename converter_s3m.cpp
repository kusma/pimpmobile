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
	
	/* check that the type is correct. just another sanity-check. */
	char name[28 + 1];
	fseek(fp, 0x0, SEEK_SET);
	fread(&name, 1, 28, fp);
	name[28] = '\0';
	printf("it's called \"%s\"\n", name);
	
	unsigned short ordnum, insnum, patnum, flags;
	fseek(fp, 0x20, SEEK_SET);
	fread(&ordnum, 1, 2, fp);
	fread(&insnum, 1, 2, fp);
	fread(&patnum, 1, 2, fp);
	fread(&flags,  1, 2, fp);
	
	printf("orders: %d\ninstruments: %d\npatterns: %d\nflags: %x\n", ordnum, insnum, patnum, flags);

	
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
