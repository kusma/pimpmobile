typedef unsigned char  u8;
typedef   signed char  s8;
typedef unsigned short u16;
typedef   signed short s16;
typedef unsigned int   u32;
typedef   signed int   s32;

typedef struct
{
	u8 note;
	u8 instrument;
	u8 volume_command;
	u8 effect_byte;
	u8 effect_parameter;
} pattern_entry_t;

/*
pattern-data are represented as an 1D array of pattern_entry
structs. For channel X, row Y, the pattern entry is:
pattern_data[X + Y*num_channels]
*/

typedef struct
{
	u16 num_rows; // 1 to 256
	pattern_entry_t *pattern_data;
	// packed pattern data structure, and its size
	u32 packed_pattern_size;
	u8 *packed_pattern_data;
	// if there are jumps to locations within a pattern,
	// we supply additional pointers to within the pattern data
	// so that we can jump to the row of the pattern without being
	// forced to decode all the preceding rows of the pattern.
	// Each pointer is treated as a 32-bit bitfield with two fields:
	// bits 11:0 - index of row pointed to by the pointer
	// bits 31:12 - pointer into the pattern: actually the number
	//    of bytes from the start of the packed pattern data to
	//    the start of the row in question.
	int pattern_pointer_count;
	u32 *pattern_pointers;
} pattern_header_t;

/*
for each channel, we specify a default pan position,
an initial channel volume, and an enum to indicate channel state
(used-normally, muted, ignored) the difference between a muted and
an ignored channel is that in a muted channel, global effects work,
but in an ignored channel, even global effects are ignored. It is safe
to ignore notes for both muted and ignored channels.
*/
typedef enum
{
	CHANNEL_NOT_MUTED,
	CHANNEL_MUTED,
	CHANNEL_IGNORED
} channel_mute_state_t;


struct channel_state_t
{
	u8 default_pan;
	u8 initial_channel_volume;
	channel_mute_state_t channel_mute_state;
};

/* data structure that is stored on a per-sample basis.
This includes the actual waveform, its format/length,
looping information, some volume/pan and vibrato parameters

*/

// loop-type for sample
typedef enum
{
	LOOP_NONE,
	LOOP_FOWARD,
	LOOP_PINGPONG,
	LOOP_PLAY_THEN_LOOP,
} loop_type_t;

// sample-format. additional formats may be added later by expanding this enum
typedef enum
{
	SAMPLE_UNSIGNED_8BIT,
	SAMPLE_SIGNED_8BIT,
	SAMPLE_UNSIGNED_16BIT,
	SAMPLE_SIGNED_16BIT
} sample_format_t;

// vibrato waveform for sample.
enum sample_vibrato_waveform_t {
	SAMPLE_VIBRATO_SINE,
	SAMPLE_VIBRATO_RAMP_DOWN,
	SAMPLE_VIBRATO_SQUARE,
	SAMPLE_VIBRATO_RANDOM, // supported by impulse-tracker but not FT2
	SAMPLE_VIBRATO_RAMP_UP // supported by FT2 but not impulse-tracker
	};

struct sample_header_t {
	char sample_name[32];
	// actual sample-data: NULL/zero length means that sample doesn't exist.
	void *sample_waveform; // pointer to actual sample waveform data
	u32 sample_length;

	// sample format
	sample_format_t sample_format;
	bool is_stereo; // is this ever actually supported in practice?????

	// looping information
	// sustain loops are active only until note-off happens;
	// sustain loops are an impulse-tracker-only feature
	loop_type_t loop_type;
	u32 loop_start;
	u32 loop_end;

	loop_type_t sustain_loop_type;
	u32 sustain_loop_start;
	u32 sustain_loop_end;

	// default values to use when playing instrument with this sample
	u8 default_volume;
	u8 default_pan_position;
	bool ignore_default_pan_position; // if true, pick default pan from channel instead?

	// the sample-note-offset is 0 for a sample that plays C-5 at 8363 Hz.
	// For other sample-freqs, the offset is stored in 16:16 format, where
	// 1 unit of the integer part corresponds to 1 halfnote up or down.
	// It is related to sample-freq with the formulas
	// note_offset = log2(sample_freq/8363) * 12 * 65536
	// sample_freq = 8363 * 2^(note_offset/(12*65536))
	s32 sample_note_offset;
	// for Amiga frequency tables, we want an additional index into
	// a an array of note->period translation maps. Each such translation
	// map contains one period for each possible notes, effectively
	// containing 120 u16 values.
	u16 note_period_translation_map_index;

	// vibrato parameters for the sample
	// Note that in XM, vibrato is specified for an instrument, so the
	// vibrato parameters must be copied to each sample at load time.
	u8 vibrato_speed;
	u8 vibrato_depth;
	u8 vibrato_sweep;
	sample_vibrato_waveform_t vibrato_waveform;
};


/* data supplied on a per-instrument basis */

// first, a data structure used to represent an envelope.
// an instrument can have a volume, panning and pitch envelopes.
struct envelope_t {
	u8 node_count; // number of nodes in the envelope (1 to 25)
	u16 node_tick[25]; // the tick at which the node is placed.
	s16 node_magnitude[25]; // the magnitude of the node
	s16 node_delta[25]; // node delta field.
	// the magnitude of the envelope at tick N:
	// first, find the last node at or before tick N - its position is M
	// then, the magnitude of the envelope at tick N is given by
	// magnitude = node_magnitude[M] + ((node_delta[M] * (N-M)) >> 8)

	// loop information for the envelope
	// The loop points are given as envelope node indices, not tick indices.
	// XM only has a sustain point, which is implemented by setting
	// sustain-loop-start and sustain-loop-end to the same value.
	bool loop_enable, sustain_loop_enable;
	u8 loop_start, loop_end;
	u8 sustain_loop_start, sustain_loop_end;
};


/* for impulse-tracker instruments, we have new-note-actions and
 duplicate-check-types and actions:
 - new-note-action is the default action to perform on already-playing notes
    when a new note is entered into a channel.
 - in addition, the new note is compared against the notes already playing
    in the channel; if the compare matches, then the duplicate-check-action
    is applied to the old note instead of the ordinary new-note-action.
*/
enum new_note_action_t {
	NNA_CUT = 0,
	NNA_CONTINUE = 1,
	NNA_NOTE_OFF = 2,
	NNA_NOTE_FADE = 3 };

enum duplicate_check_type_t {
	DCT_OFF = 0,
	DCT_NOTE = 1,
	DCT_SAMPLE = 2,
	DCT_INSTRUMENT = 3 };

enum duplicate_check_action_t {
	DCA_CUT = 0,
	DCA_NOTE_OFF = 1,
	DCA_NOTE_FADE = 2 };


struct instrument_t {
	char instrument_name[32];

	// pointers to the envelope data structures
	// for the instrument. NULL pointers means that the envelopes
	// don't exist
	envelope_t *volume_envelope;
	envelope_t *panning_envelope;
	envelope_t *pitch_envelope;
	u16 fadeout_rate;

	// sone data fields for impulse-tracker only
	new_note_action_t   new_note_action;
	duplicate_check_type_t   duplicate_check_type;
	duplicate_check_action_t   duplicate_check_action;
	s8 pitch_pan_separation; // no idea what this one does
	u8 pitch_pan_center; // not this on either; this one seems to be a note index


	u8 sample_count; // number of samples tied to instrument
	sample_header_t *sample_headers; // pointer to an array of sample headers for the instrument

	// for each possible note of the instrument, we have a sample index
	// tied to it.
	u8 instrument_sample_map[120];
};


// helper structure to aid the translation of notes into
// periods for one sample-frequency.
struct note_period_translation_table_t {
	u16 translation_table[120]; };



/* toplevel data structure for the module */
typedef struct
{
	char module_name[32];

	// special flags affecting the playback of the module
	bool use_linear_frequency_table; // if false, use amiga frequency table
	u16 period_low_clamp, period_high_clamp; // clamps on note periods when playing module

	// a number of miscellaneous compatibility flags
	bool instrument_vibrato_use_linear_frequency_table; // if false, use amiga frequency table
	bool volume_slide_in_tick0;
	bool vibrato_in_tick0;
	bool vol0_optimizations; // if sample volume is 0 for 2 frames, kill note
	bool tremor_extra_delay; // add 1 extra tick of delay for tremor on/off-time
	bool tremor_has_memory; // affects what happens if tremor is given with parameter-byte 00
	bool retrig_kills_note; // if true, multi-retrig kills note at end of frame, else note keeps playing
	bool note_cut_kills_note; // if false, note-cut merely sets volume to 0
	bool allow_nested_loops; // if true, allow nested loop, else allow loop start/end in different channels
	bool retrig_note_source_is_period; // retrig w/o note: use channel period or period of last issued note?
	bool delay_global_volume; // false: apply global-vol NOW, true: apply only with new notes.
	bool sample_offset_clamp; // if true, sample offset past sample-end clamps to sample-end, else sample offset past note-end kills note
	bool tone_porta_share_memory; // if true, tone porta shares memory with porta up & down.
	bool remember_tone_porta_target; // if true, tone porta target is remembered even if new notes are issued inbetween.

	// the playing order. The order is an array of ubytes, each with 1
	// pattern index; in the order, the special value 254 is used
	// by ST3 to signify an empty entry (proceed to the next one, repeat
	// as many times as necessary) and 255 the end of the order.
	// At end-of-order, the module either quits or jumps to the repeat-position
	// of the order.
	u16 play_order_length;
	u8  play_order_repeat_position;
	u8  *play_order;

	// channel configuration
	u8 num_channels; // number of channels in the pattern-data of the module.
	channel_state_t *channel_states; // array of per-channel configuration data

	// the patterns of the module.
	u8 pattern_count;
	pattern_header_t *patterns; // pointer to an array of pattern headers

	// the instruments of the module
	u8 instrument_count;
	instrument_t *instruments;

	// pointer to an array of helper structures to help
	// translation form notes to amiga frequency table ticks.
	// One such table is supplied for each sample frequency
	// used in the module;
	u16 translation_table_count;
	note_period_translation_table_t *translation_table_array;

	// initial player settings for the module
	u8 initial_global_volume; // 0..64
	u8 initial_tempo;
	u8 initial_bpm;
} module_t;
