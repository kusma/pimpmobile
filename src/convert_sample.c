/* convert_sample.c -- Sample converter code
 * Copyright (C) 2006-2007 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include "pimp_base.h"
#include "pimp_debug.h"
#include "convert_sample.h"

void pimp_convert_sample(void *dst, enum pimp_sample_format dst_format, void *src, enum pimp_sample_format src_format, size_t sample_count)
{
	size_t i;
	
	ASSERT(NULL != src);
	ASSERT(NULL != dst);
	ASSERT(src != dst);

	ASSERT(dst_format == PIMP_SAMPLE_U8);
	
	for (i = 0; i < sample_count; ++i)
	{
		s32 new_sample = 0;
		
		/* fetch data, and get it to PIMP_SAMPLE_S16 format */
		switch (src_format)
		{
			case PIMP_SAMPLE_U8:    new_sample = ((s32)((s8*) src)[i] << 8) - (1 << 15); break;
			case PIMP_SAMPLE_S8:    new_sample = ((s32)((s8*) src)[i] << 8) - (0 << 15); break;
			case PIMP_SAMPLE_U16:   new_sample = ((s32)((u16*)src)[i] << 0) - (1 << 15); break;
			case PIMP_SAMPLE_S16:   new_sample = ((s32)((s16*)src)[i] << 0) - (0 << 15); break;
			default: ASSERT(FALSE);
		}
		
		/* write back from S16 format */
		switch (dst_format)
		{
			case PIMP_SAMPLE_U8:  ((u8*) dst)[i] = (u8) ((new_sample + (1 << 15)) >> 8); break;
			case PIMP_SAMPLE_S8:  ((s8*) dst)[i] = (s8) ((new_sample + (0 << 15)) >> 8); break;
			case PIMP_SAMPLE_U16: ((u16*)dst)[i] = (u16)((new_sample + (1 << 15)) >> 0); break;
			case PIMP_SAMPLE_S16: ((s16*)dst)[i] = (s16)((new_sample + (0 << 15)) >> 0); break;
			default: ASSERT(FALSE);
		}
	}
}

