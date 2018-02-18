/* pimp_sample_bank.h -- Sample databse for Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_SAMPLE_BANK_H
#define PIMP_SAMPLE_BANK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pimp_base.h"
#include "pimp_debug.h"

struct pimp_sample_bank
{
	void       *data;
	pimp_size_t size;
};

void pimp_sample_bank_init(struct pimp_sample_bank *sample_bank);

static INLINE void *pimp_sample_bank_get_sample_data(const struct pimp_sample_bank *sample_bank, int offset)
{
	ASSERT(NULL != sample_bank);
	ASSERT(offset < sample_bank->size);
	
	return (void*)((u8*)sample_bank->data + offset);
}

int pimp_sample_bank_find_sample_data(const struct pimp_sample_bank *sample_bank, void *data, pimp_size_t len);
int pimp_sample_bank_insert_sample_data(struct pimp_sample_bank *sample_bank, void *data, pimp_size_t len);

#ifdef __cplusplus
}
#endif

#endif /* PIMP_SAMPLE_BANK_H */
