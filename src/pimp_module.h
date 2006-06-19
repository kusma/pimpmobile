#ifndef PIMP_MODULE_H
#define PIMP_MODULE_H

#include "pimp_instrument.h"

typedef struct
{
	u32 data_ptr;
	u16 row_count;
} pimp_pattern;

/* packed, because it's all bytes. no member-alignment or anything needed */
typedef struct __attribute__((packed))
{
	u8 pan;
	u8 volume;
	u8 mute;
} pimp_channel;

typedef struct
{
	char name[32];
	
	u32 flags;
	u32 reserved; /* for future flags */
	
	// these are offsets relative to the begining of the pimp_module_t-structure
	/* TODO: make these relative to their storage, and make a generic getter for those kinds of pointers */
	u32 order_ptr;
	u32 pattern_ptr;
	u32 channel_ptr;
	u32 instrument_ptr;
	
	u16 period_low_clamp;
	u16 period_high_clamp;
	u16 order_count;
	
	u8  order_repeat;
	u8  volume;
	u8  tempo;
	u8  bpm;
	
	u8  instrument_count;
	u8  pattern_count;
	u8  channel_count;
} pimp_module;

#include "pimp_debug.h"
#include "pimp_internal.h"

/* pattern entry */
STATIC INLINE pimp_pattern_entry *get_pattern_data(pimp_pattern *pat)
{
	ASSERT(pat != NULL);
	return (pimp_pattern_entry*)get_ptr(&pat->data_ptr);
}

/* getters for pointer-based properties */

STATIC INLINE int get_order(const pimp_module *mod, int i)
{
	ASSERT(mod != NULL);
	return ((char*)get_ptr(&mod->order_ptr))[i];
}

STATIC INLINE pimp_pattern *get_pattern(const pimp_module *mod, int i)
{
	ASSERT(mod != NULL);
	return &((pimp_pattern*)get_ptr(&mod->pattern_ptr))[i];
}

STATIC INLINE pimp_channel &get_channel(const pimp_module *mod, int i)
{
	ASSERT(mod != NULL);
	return ((pimp_channel*)get_ptr(&mod->channel_ptr))[i];
}

STATIC INLINE pimp_instrument *get_instrument(const pimp_module *mod, int i)
{
	ASSERT(mod != NULL);
	return &((pimp_instrument*)get_ptr(&mod->instrument_ptr))[i];
}

#endif /* PIMP_MODULE_H */
