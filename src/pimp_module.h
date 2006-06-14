#ifndef PIMP_MODULE_H
#define PIMP_MODULE_H

#include "pimp_instrument.h"

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

#endif /* PIMP_MODULE_H */
