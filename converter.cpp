#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"

void print_usage()
{
	printf("usage: converter filenames\n");
	exit(1);
}

module_t *load_module_XM(FILE *fp);

int main(int argc, char *argv[])
{
	if (argc < 2) print_usage();
	
	for (int i = 1; i < argc; ++i)
	{
		printf("loading module: %s\n", argv[i]);
		FILE *fp = fopen(argv[i], "rb");
		if (!fp) print_usage();
		
		module_t *mod;
		
		if (!(mod = load_module_XM(fp)))
		{
			printf("failed to load!\n");
		}
		
		fclose(fp);
	}
}
