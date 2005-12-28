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
	
/*
	IT ONLY (later)
	u32 sustain_loop_start;
	u32 sustain_loop_end;
*/

	u8  volume;
	s8  finetune;
	u8  loop_type;
	u8  pan;
	s8  rel_note;
	
	u8 vibrato_speed;
	u8 vibrato_depth;
	u8 vibrato_sweep;
	u8 vibrato_waveform;

/*
	IT ONLY (later)
	u8  sustain_loop_type;
*/

} pimp_sample_t;

typedef struct
{
	s32 period;        /* signed so we can check for underfow */
	s32 porta_speed;
	
	u8  note;
	u8  effect;
	u8  effect_param;
} pimp_channel_state_t;

typedef enum
{
	EFF_NONE       = 0x00,
	EFF_PORTA_UP   = 0x01,
	EFF_PORTA_DOWN = 0x02,
	EFF_PORTA_NOTE = 0x03,
	EFF_VIBRATO    = 0x04,
	/* todo: fill in the rest */
} pimp_effect_t;

/* packed, because it's all bytes. no member-alignment or anything needed */
typedef struct __attribute__((packed))
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
	// this is an offset relative to the begining of the pimp_module_t-structure
	u32 data_ptr;
	
	u16 row_count;
} pimp_pattern_t;

/* packed, because it's all bytes. no member-alignment or anything needed */
typedef struct __attribute__((packed))
{
	u8 pan;
	u8 volume;
	u8 mute;
} pimp_channel_t;

typedef struct
{
	u16 node_tick[25];
	s16 node_magnitude[25];
	s16 node_delta[25];
	u8 node_count;
	u8 flags; // bit 0: loop enable, bit 1: sustain loop enable
	u8 loop_start, loop_end;
	u8 sustain_loop_start, sustain_loop_end;
} pimp_envelope_t;

typedef struct
{
	u32 sample_ptr;
	u32 volume_envelope_ptr;
	u32 panning_envelope_ptr;
	u32 pitch_envelope_ptr;

	u16 volume_fadeout;
	u8 sample_count;                 /* number of samples tied to instrument */

#if 0
	// IT ONLY (later)
	new_note_action_t        new_note_action;
	duplicate_check_type_t   duplicate_check_type;
	duplicate_check_action_t duplicate_check_action;
	
	// UNKNOWN
	s8 pitch_pan_separation; // no idea what this one does
	u8 pitch_pan_center;     // not this on either; this one seems to be a note index
#endif
	u8 sample_map[120];
} pimp_instrument_t;

typedef struct
{
	char name[32];
	
	u32 flags;
	u32 reserved; /* for future flags */

	// these are offsets relative to the begining of the pimp_module_t-structure
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
} pimp_module_t;

#endif /* INTERNAL_H */
