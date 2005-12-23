#ifndef INTERNAL_H
#define INTERNAL_H

#include "config.h"

typedef struct
{
	/* offset relative to sample-bank */
	u32 data_ptr;

	u32 length;
	u32 loop_start;
	u32 loop_length;
	u8  volume;
	s8  finetune;
	u8  type;
	u8  pan;
	s8  rel_note;
} pimp_sample_t;

typedef struct
{
	u8  note;
	u8  effect;
	u8  effect_param;
	s32 period;        /* signed so we can check for underfow */
	s32 porta_speed;   /* signed so we can check for underfow */
} pimp_channel_t;

typedef enum
{
	EFF_NONE       = 0x00,
	EFF_PORTA_UP   = 0x01,
	EFF_PORTA_DOWN = 0x02,
	EFF_PORTA_NOTE = 0x03,
	EFF_VIBRATO    = 0x04,
	/* todo: fill in the rest */
} pimp_effect_t;

typedef struct
{
	u8 note;
	u8 instrument;
	u8 volume_command;
	u8 effect_byte;
	u8 effect_parameter;
} pimp_pattern_entry_t;

enum
{
	FLAG_LINEAR_PERIODS          = (1 <<  0),
	FLAG_LINEAR_VIBRATO          = (1 <<  1),
	FLAG_VOL_SLIDE_TICK0         = (1 <<  2),
	FLAG_VIBRATO_TICK0           = (1 <<  3),
	FLAG_VOL0_OPTIMIZE           = (1 <<  4),
	FLAG_TEMOR_EXTRA_DELAY       = (1 <<  5),
	FLAG_TEMOR_MEMORY            = (1 <<  6),
	FLAG_RETRIG_KILLS_NOTE       = (1 <<  7),
	FLAG_NOTE_CUT_KILLS_NOTE     = (1 <<  8),
	FLAG_ALLOW_NESTED_LOOPS      = (1 <<  9),
	FLAG_RETRIG_NOTE_PERIOD      = (1 << 10),
	FLAG_DELAY_GLOBAL_VOLUME     = (1 << 11),
	FLAG_SAMPLE_OFFSET_CLAMP     = (1 << 12),
	FLAG_PORTA_NOTE_SHARE_MEMORY = (1 << 13),
	FLAG_PORTA_NOTE_MEMORY       = (1 << 14),
};

typedef struct
{
	char name[32];
	
	u32 flags;
	
	u16 period_low_clamp;
	u16 period_high_clamp;
	u16 order_length;
	
	u8  order_repeat;
	u8  volume;
	u8  tempo;
	u8  bpm;
	
	u8  instrument_count;
	u8  pattern_count;
	u8  channel_count;
	
	// these are offsets relative to the begining of the pimp_module_t-structure
	unsigned order_ptr;
	unsigned pattern_data_ptr;
	unsigned channel_ptr;
	unsigned instrument_ptr;
} pimp_module_t;

#endif /* INTERNAL_H */
