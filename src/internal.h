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
	s16 fine_tune;
	s16 rel_note;

	u8  volume;
	u8  loop_type;
	u8  pan;
	
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
	s32 period;
	s32 final_period; /* signed so we can check for underfow */
	s32 porta_target;
	s8  porta_speed;
	s8  volume_slide_speed;

	s8  volume;
	u8  pan;
	
	u8  note;
	u8  effect;
	u8  effect_param;
} pimp_channel_state_t;

typedef enum
{
	EFF_NONE                    = 0x00,
	EFF_PORTA_UP                = 0x01,
	EFF_PORTA_DOWN              = 0x02,
	EFF_PORTA_NOTE              = 0x03,
	EFF_VIBRATO                 = 0x04,
	EFF_PORTA_NOTE_VOLUME_SLIDE = 0x05,
	EFF_VIBRATO_VOLUME_SLIDE    = 0x06,
	EFF_TREMOLO                 = 0x07,
	EFF_SET_PAN                 = 0x08,
	EFF_SAMPLE_OFFSET           = 0x09,
	EFF_VOLUME_SLIDE            = 0x0A,
	EFF_JUMP_ORDER              = 0x0B,
	EFF_SET_VOLUME              = 0x0C,
	EFF_BREAK_ROW               = 0x0D,
	EFF_MULTI_FX                = 0x0E,
	EFF_TEMPO                   = 0x0F,
	EFF_SET_GLOBAL_VOLUME       = 0x10,
	EFF_GLOBAL_VOLUME_SLIDE     = 0x11,
	EFF_KEY_OFF                 = 0x14,
	EFF_SET_ENVELOPE_POSITION   = 0x15,
	EFF_PAN_SLIDE               = 0x19,
	EFF_MULTI_RETRIG            = 0x1B,
	EFF_TREMOR                  = 0x1D,
	EFF_SYNC_CALLBACK           = 0x20,
	/* missing 0x21 here, multi-command (X1 = extra fine porta up, X2 = extra fine porta down) */
	EFF_ARPEGGIO                = 0x24,
	EFF_SET_TEMPO               = 0x25,
	EFF_SET_BPM                 = 0x26,

/*
	TODO: fill in the rest
	27xx: set channel volume (impulse-tracker)
	28xx: slide channel volume (impulse-tracker)
	29xx: panning vibrato (impulse-tracker)
	2Axx: fine vibrato
	2Bxy: multi-effect (impulse-tracker only)
		5x: panning vibrato waveform( sane as vibrato/tremolo waveform)
		70: past note cut
		71: past note off
		72: past note fade
		73: set NNA to note cut
		74: set NNA to continue
		75: set NNA to note-off
		76: set NNA to note-fade
		77: turn off volume envelope
		78: turn on volume envelope
		79: turn off panning envelope
		7A: turn on panning envelope
		7B: turn off pitch envelope
		7C: turn on pitch envelope
		91: "set surround sound" ???
	
*/
} pimp_effect_t;

typedef enum
{
	EFF_AMIGA_FILTER           = 0x0,
	EFF_FINE_VOLUME_SLIDE_UP   = 0xA,
	EFF_FINE_VOLUME_SLIDE_DOWN = 0xB,
	EFF_NOTE_DELAY             = 0xD,
/*
		0x: Amiga filter on/off (ignored; according to everybody, it sucks)
		1x: fine porta up
		2x: fine porta down
		3x: glissando control (0=off, 1=on)
		4x: set vibrato waveform/control:
			0=sine, 1=ramp-down, 2=square, 3=random
			+4: vibrato waveform position is not reset on new-note
		5x: set fine-tune for note
		6x: pattern loop command (x=0 marks start of loop,
			x != 0 marks end of loop, with x=loop iterations to apply)
		7x: set tremolo control (same syntax as vibrato control)
		8x: normally unused; Digitrakker uses this command
			to control the looping mode of the currently-playing sample
			(0=off, 1=forward, 3=pingpong)
		9x: retrig note
		Ax: fine volume slide up
		Bx: fine volume slide down
		Cx: note cut after x ticks
		Dx: note delay by x ticks
		Ex: pattern delay by x frames
		Fx: FunkInvert (does anyone actually have an idea what this does?)
*/
} pimp_multi_effect_t;

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

#if 0
	// IT ONLY (later)
	u32 pitch_envelope_ptr;
#endif

	u16 volume_fadeout;
	
	u8 sample_count;                 /* number of samples tied to instrument */
	u8 sample_map[120];

#if 0
	// IT ONLY (later)
	new_note_action_t        new_note_action;
	duplicate_check_type_t   duplicate_check_type;
	duplicate_check_action_t duplicate_check_action;
	
	// UNKNOWN
	s8 pitch_pan_separation; // no idea what this one does
	u8 pitch_pan_center;     // not this on either; this one seems to be a note index
#endif
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
