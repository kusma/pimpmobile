#include "pimp_sample_bank.h"
#include "pimp_debug.h"

#include <stdlib.h>

void pimp_sample_bank_init(pimp_sample_bank *sample_bank)
{
	sample_bank->data = NULL;
	sample_bank->size = 0;
}

int pimp_sample_bank_find_sample_data(const pimp_sample_bank *sample_bank, void *data, pimp_size_t len)
{
	int i;
	ASSERT(NULL != sample_bank);
	
	if (len > sample_bank->size) return -1; /* not found */
	for (i = 0; i < (sample_bank->size - len) + 1; ++i)
	{
		if (0 == memcmp(data, pimp_sample_bank_get_sample_data(sample_bank, i), len)) return i;
	}
	
	return -1; /* not found */
}

int pimp_sample_bank_insert_sample_data(pimp_sample_bank *sample_bank, void *data, pimp_size_t len)
{
	int pos = sample_bank->size;
	
	/* allocate more memory */
	void *new_data = realloc(
		sample_bank->data,
		sample_bank->size + len
	);
	if (NULL == new_data) return -1;
	
	/* update structure */
	sample_bank->data = new_data;
	sample_bank->size += len;
	
	/* copy data */
	{
		void *dst = (void*)((int)sample_bank->data + pos);
		memcpy(dst, data, len);
	}
	
	return pos;
}
