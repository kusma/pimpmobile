/*

data structure for the converter's internal represntation of
a module. The MOD, S3M and XM loaders load into this format.

*/



/* The pattern-entry contains 5 bytes:
 - note, instrument, volume_command, effect_byte, effect_parameter

 - The note is represented as a number in the range 1-120, ranging
    from C-0 to B-9.  C-5 (value=61) represents the default note, which is
    played as 8363 Hz in MOD/XM (modulo note-shift) and sample_freq in S3M.
    The number 121 is used to represent key-off (S3M, XM)
    The number 0 is used to indicate that there is no note present in channel.
 - The instrument is an integer in the range 1 to 255, 0 means no instrument.
 - The volume column is actually an effect byte, which obeys FT2 format (hex):
     00 = empty
     01..0F: porta up x-1(impulse-tracker; FT2 cannot produce this value)
     10..50: set volume to value minus 0x10.
     51..5F: porta down x-1(impulse-tracker; FT2 cannot produce this value)
     6x: volume slide up
     7x: volume slide down
     8x: fine volume slide up
     9x: fine volume slide down
     Ax: set vibrato speed
     Bx: perform vibrato
     Cx: set panning position to xx
     Dx: panning slide left
     Ex: panning slide right
     Fx: tone portamento (note, the porta is 16x the strength of effects
          column porta).
   The column is never used for MOD; ST3 uses it for set volume only.
   Note that for impulse-tracker, serious remapping of byte values is
   necessary.

 - The effect column has an effect byte and a parameter byte. The supported
    effect/parameter bytes are:

    (0..F for the effect byte are the usual hex values,
      the values continue naturally upwards with G,H,I...)
    0xx: no effect at all (warning: MOD/XM uses effect byte 0 for
         arpeggio; they need to be remapped to effect 24)
    1xx: porta up
    2xx: porta down
    3xx: tone porta
    4xy: vibrato: x=rate, y=depth
    5xx: tone porta + volume-slide (command byte is for slide, the tone porta
           is remembered)
    6xx: vibrato + volume-slide (comamnd byte is for slide, the vibrato
           is remembered)
    7xy: tremolo: x=rate, y=depth
    8xx: set panning position (8xx is strictly speaking ignored in Protracker
          MOD files, but we are going to obey it anyway)
    9xx: set sample offset (obeyed only together with new note)
    Axx: volume slide command. The command byte follows the format
          of the ST3 volume command Dxx, implying that for MOD/XM,
          we must restore the volume byte to a valid value
0B	Bxx: jump to order (for MOD/S3M/XM but not IT, we must convert
           this value from BCD to binary when loading pattern)
0C	Cxx: set volume
0D	Dxx: pattern break
0E	Exy: MultiFX: x = effect selector: values of x:
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

0F	Fxx: set tempo/speed (00-1F: set tempo, 20-FF: set BPM)
10	Gxx: set global volume (00..40)
11	Hxy: global volume slide command
12	Ixx:
13	Jxx:
14	Kxx: note key-off after xx ticks (not really documented)
15	Lxx: set envelope position (# of ticks)
16	M
17	N
18	O
19  Pxy: panning slide (x=left slide, y=right slide)
1A  Q
1B  Rxy: multi retrig note:
          x=volume modifier per retrig
             0=none       8="unused"
             1=-1         9=+1
             2=-2         A=+2
             3=-4         B=+4
             4=-8         C=+8
             5=-16        D=+16
             6=*2/3       E=*3/2
             7=*1/2       F=*2
          y=ticks per retrig
1C	S
1D  Txy: Tremor x=on-time, y=off-time (+1 for S3M, +2 for XM)
1E	U
1F	V
20	Wxx: Sync Callback
21  Xxy: Multi command again, for values of X:
          1y: extra fine portamento up
          2y: extra fine portamento down
22	Y
23	Z

   -- following effect bytes are given in hex.
   24xx: arpeggio (0=remembers previous value)
   25xx: set tempo (not limited to range 0..1F unlike effect Fxx)
   26xx: set BPM (not limited to range 20..FF unlike effect Fxx)
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

    The effect bytes for all the 1-byte volume (ordinary volume, global volume,
    channel volume) and panning slide commands work like
    in ScreamTracker3, with the value being interpreted as follows (pick the
    first one that applies):
     - 00 = remember previous value, then continue interpreting.
     - x0 = slide up by x
     - 0x = side down by x
     - xF = fine slide up by x
     - Fx = fine slide down by x
     - other value = slide down by low digit
    For MOD/FT2 volume slides, if both Up speed and Down speed are set, the
    Up speed must be zeroed out.

    When loading MOD/XM, the effect column can be kept mostly unchanged
    with the following exceptions:
     - Arpeggio (0xy for xy != 0 in MOD/XM) is remapped from effect
        0 to effect 0x24
     - Jump-to-order Bxx needs to have its argument converted from BCD to
        Binary.
     - Volume Slide Axy needs to be sanitized (if y != 0, then x is set to 0
        for XM, not clear what MOD does). This also applies to Global
        Volume slide and Panning Slide (Hxx and Pxx)
     - Several MultiFX effects are unimplemented in FT2:
         this applies to E0x, E3x, E8x, EFx - should these be cleaned
         from the effects columns or left in or should we assume that the
         musician that put them in did so intentionally?
    For ScreamTracker/ImpulseTracker, the effects in the effect columns usually
    can keep their parameters unchanged, but the effect byte needs to undergo
    considerable remapping.

*/

struct pattern_entry_t {
	u8 note,
	u8 instrument,
	u8 volume_command,
	u8 effect_byte,
	u8 effect_parameter };

/*
pattern-data are represented as an 1D array of pattern_entry
structs. For channel X, row Y, the pattern entry is:
pattern_data[X + Y*num_channels]
*/

struct pattern_header_t {
	u16 num_rows; // 1 to 256
	pattern_entry *pattern_data;
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
	};

/*
for each channel, we specify a default pan position,
an initial channel volume, and an enum to indicate channel state
(used-normally, muted, ignored) the difference between a muted and
an ignored channel is that in a muted channel, global effects work,
but in an ignored channel, even global effects are ignored. It is safe
to ignore notes for both muted and ignored channels.
*/
enum channel_mute_state_t {
	CHANNEL_NOT_MUTED,
	CHANNEL_MUTED,
	CHANNEL_IGNORED };


struct channel_state_t {
	u8 default_pan;
	u8 initial_channel_volume;
	channel_mute_state_t channel_mute_state; };

/* data structure that is stored on a per-sample basis.
This includes the actual waveform, its format/length,
looping information, some volume/pan and vibrato parameters

*/

// loop-type for sample
enum loop_type_t {
	LOOP_NONE,
	LOOP_FOWARD,
	LOOP_PINGPONG };

// sample-format. additional formats may be added later by expanding this enum
enum sample_format_t {
	SAMPLE_UNSIGNED_8BIT,
	SAMPLE_SIGNED_8BIT,
	SAMPLE_UNSIGNED_16BIT,
	SAMPLE_SIGNED_16BIT };

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
	void *sample_waveform, // pointer to actual sample waveform data
	u32 sample_length,

	// sample format
	sample_format_t sample_format;
	bool is_stereo; // is this ever actually supported in practice?????

	// looping information
	// sustain loops are active only until note-off happens;
	// sustain loops are an impulse-tracker-only feature
	loop_mode_t loop_type;
	u32 loop_start;
	u32 loop_end;
	loop_type_t sustain_loop_type;
	u32 sustain_loop_start:
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
	}


/* data supplied on a per-instrument basis */

// first, a data structure used to represent an envelope.
// an instrument can have a volume, panning and pitch envelopes.
struct envelope_t {
	u8 node_count; // number of nodes in the envelope (1 to 25)
	u8 node_tick[25]; // the tick at which the node is placed.
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
struct module_t {

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
	u8 play_order_length;
	u8 play_order_repeat_position;
	u8 *play_order;

	// channel configuration
	u8 num_channels; // number of channels in the pattern-data of the module.
	channe_state_t *channel_states; // array of per-channel configuration data

	// the patterns of the module.
	u8 pattern_count;
	pattern_header *patterns; // pointer to an array of pattern headers

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
	};





// prototypes for toplevel functions
// that load and store modules
module_t *load_module_XM();
module_t *load_module_MOD();
module_t *load_module_S3M();
module_t *load_module_IT();
void store_module_FXM(module_t *);
void delete_module(module_t *);

// utility functions that are likely to be shared between multiple load/store
// paths
void delete_empty_channels(module_t *);
void reduce_samples_to_8bit(module_t *);
void truncate_samples(module_t *);
void build_note_period_translation_tables(module_t *);
void compress_patterns(module_t *);
void make_pattern_pointers(module_t *);


