#ifndef CONVERT_SAMPLE_H
#define CONVERT_SAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* for size_t */
#include "../src/pimp_base.h"
#include "../src/pimp_debug.h"

enum pimp_sample_format
{
	PIMP_SAMPLE_U8,
	PIMP_SAMPLE_S8,
	PIMP_SAMPLE_U16,
	PIMP_SAMPLE_S16
};

static INLINE size_t pimp_sample_format_get_size(enum pimp_sample_format format)
{
	switch (format)
	{
		case PIMP_SAMPLE_U8: return sizeof(u8);
		case PIMP_SAMPLE_S8: return sizeof(s8);
		case PIMP_SAMPLE_U16: return sizeof(u16);
		case PIMP_SAMPLE_S16: return sizeof(s16);
		default:
			ASSERT(FALSE);
			return 0;
	}
}

void pimp_convert_sample(void *dst, enum pimp_sample_format dst_format, void *src, enum pimp_sample_format src_format, size_t size);

#ifdef __cplusplus
}
#endif

#endif
