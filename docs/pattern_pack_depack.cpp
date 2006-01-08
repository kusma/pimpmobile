/*
Pattern compression/decompression routines.

Observations:
 - It is common to have a track in a pattern where:
    - note is same for every note
    - instrument is same for every note
    - no effects are used
    - no volume commands are used
 - it is also common to have a track where not both
    note AND effect are present at the same time.

So, for each pattern, we store once for each track:
 - Bitmap:
    - bit 0: default note for all notes in track
    - bit 1: default instrument for all notes in track
    - bit 2: track does not contain volume commands
    - bit 3: track does not contain effect commands
 - after one bitmap, up to two bytes follow: default note and default instrument

We then have a bitmap of empty/nonempty rows; there is 1 bit per row,
and the bitmap size is number of rows in pattern divided by 8 (rounded up)
Then, for each nonempty row, we have:
 - Bitmap of nonempty slots in the row. This bitmap has a size of 1 bit per channel
   rounded up to nearest integer byte.
What follows next is a sequence of packed notes, one for each nonempty slot.
Each packed note has the following format:
 - 1 byte for note, unless default note is supplied.
 - 1 byte for instrument, unless default instrument is supplied
 - 1 byte for volume command, if any command present
 - 1 byte for effect command, if effect commands present. MSB is enable for effect byte
 - 1 byte for effect byte, if effect command present and effect command MSB is 1.
    (Otherwise the effect byte is zero)



Depacking of a pattern is a serial process. The packed data header
 (track-states and nonempty-row-bitmaps) must always be processed first.
After that, it is possible to generate a pointer to each row of data
(this is not done by default).

In order to facilitate pattern break and loop commands, we need to
add a list of row pointers to the pattern. For loop commands, we can just scan
the pattern for the loop-start command and emit a row pointer for
that row. For pattern-break, the procedure is a bit harder:
 - Scan the playing order for all cases where the current pattern
   is followed by another pattern. Then add a row pointer to that other pattern,
   if it doesn't have one at the same location already. At the end of the
   playing order, if the last pattern contains a break, then add a row pointer
   to the repeat-start pattern.
Each row-pointer requires 32 bits of data: the low 12 bits are pattern
row index; the high 20 bits are the pointer. The pointer is a 20-bit address
relative to the start of the packed pattern data. When there is a jump
into the pattern, the row pointers are scanned until a hit is found.


Thus, the pattern structure needs to be:
 - Number of rows (1 to 255)
 - Length of packed pattern data (up to ~80 Kbytes)
 - Number of row-pointers
 - <actual row-pointers>
 - <packed pattern data>

*/


// pattern_note struct, containing 1 note with associated effects.
// The decode_row function expects a pointer to
// an array of these structs, one for each channel.
struct pattern_note {
	u8 channel, // channel that note applies to
	u8 note, // actual note. 1-120= note C-0 to B-9, 127 = key-off, 0=empty
	u8 instrument, // 1 to 255; 0 means empty
	u8 vol_command, // 0 to 255
	u8 effect_command, // 0 to 127
	u8 effect_byte // 0 to 255
	};

// track_state is state that is maintained on a per-channel basis while
// depacking a pattern.
struct track_state {
	u8 flags; // track bitmap above
	u8 default_note;
	u8 default_instrument;
	u8 padding;
	} *track_depack_states; // 1 state for each channel in the current pattern


// function returns the number of actual notes returned in the pattern_note struct.
int decode_row(
	int channels_div8, // channels divided by 8 (rounded up)
	int pattern_pos, // position in pattern (row)
	u8 *current_pos, // current byte position in pattern
	u8 **next_pos, // current byte position, after this row has been processed.
	pattern_note *pt // buffer to output pattern notes to
	)
	{
	int i;
	*next_pos = current_pos;
	if( nonempty_row_bitmap[pattern_pos >> 3] & (1 << (pattern_pos & 7)) == 0 )
		return 0;
	int num_notes = 0;
	u8 *note_bitmap = current_pos;
	current_pos += channels_div8;
	for(i=0; i<channels_div8; i++)
		{
		u32 p = *note_bitmap++;
		int j = i << 3;
		track_state *ts = track_depack_state + j;
		while(p)
			{
			if(p & 1)
				{
				// we have a note. depack it.
				pt->channel = j;
				int flags = ts->flags;
				pt->note = flags & 1 ? *current_pos++ : ts->default_note;
				pt->instrument = flags & 2 ? *current_pos++ : ts->default_instrument;
				pt->vol_command = flags & 4 ? *current_pos++ : 0;
				int command = flags & 8 ? *current_pos++ : 0;
				pt->effect_command = command & 0x7F;
				pt->effect_byte = command & 0x80 ? *current_pos++ : 0;
				num_notes++;
				pt++;
				}
			j++;
			ts++;
			p >>= 1;
			}
		}
	*next_pos = current_pos;
	return num_notes;
	}


// when starting processing of a pattern, prepare the track-state array
// and a pointer to the empty-row bitmap.
void prepare_pattern_processing(
	u8 *start_pos, // start position of packed data
	u8 **next_pos, // position to continue fetching pattern-data from
	int channels // channels in pattern
	)
	{
	track_state *ts = track_depack_states;
	int i;
	for(i=0;i<channels; i++)
		{
		int flags = *current_pos++;
		ts->flags = flags;
		if(flags & 1)
			ts->default_note = *current_pos++;
		if(flags & 2)
			ts->default_instrument = *current_pos++;
		ts++;
		}
	nonempty_row_bitmap = current_pos;
	current_pos += (channels + 7) >> 3;
	*next_pos = current_pos;
	}




/*
given a pattern in tabular form, as an array of pattern_note,
pack a pattern into the compressed format.
This goes in 2 stages:
  1: scan for default-notes and default-instruments, and record them
     also scan for presence of volume commands and effect commands
     scan for empty rows, amd make an empty-row-bitmap too.
  2: for each nonempty row:
      - allocate a slot bitmap
      - for each non-empty note, set the bit in the slot bitmap
      - and encode the note into the packed stream.


To access the initial unpacked pattern at row X, channel Y:
  pattern[X*channels+Y].


*/

struct unpacked_pattern {
	int rows;
	int channels;
	pattern_note *pattern_data;
	};


void pack_pattern(
	int rows, // rows of pattern to pack
	int channels, // channels of pattern to pack
	pattern_note *pattern, // actual unpacked pattern data to pack
	int *length, // length of packed data
	u8 **packed_data // packed data block
	)
	{
	int i,j;

	// worst-case packed data usage is:
	//  3*chan                 per-channel flags
	//  + CEIL(rows/8)         row-empty bitmap
	//  + rows * CEIL(chan/8)  note-empty bitmaps
	//  + rows * chan * 5      note data

	int max_size =
		3* channels
		+ (rows + 7)/8
		+ rows * (chan+7)/8
		+ rows * chan * 5;

	u8 *packed_data_area = calloc(max_size, 1);

	struct pack_track_state {
		u8 found_volume_command;
		u8 found_effect_command;
		u8 found_note_state; // 0=no note, 1=one note, 2=multiple notes
		u8 first_note_found;
		u8 found_instrument_state; // 0=no instrument, 1=one instrument, 2=multiple instruments
		u8 first_instrument_found;
		};

	// allocate and zero out track state flags and bitmap of nonempty rows.
	pack_track_state *pack_states = calloc( pack_track_state, channels );
	u8 *nonempty_row_bitmap = calloc( rows, 1 );


	// 1st analysis pass: record track state flags and nonempty row flags
	for(i=0;i< rows; i++)
		{
		for(j=0; j< channels;j++)
			{
			pattern_note *pat2 = pattern + i*channels + j;
			// if the pattern entry is all 0s, we will not process it.
			if(pat2->note == 0
				&& pat2->instrument == 0
				&& pat2->vol_command == 0
				&& pat2->effect_command == 0
				&& pat2->effect_byte == 0)
				continue;
			nonempty_row_bitmap[i] = 1;
			// check if we have found the first note in the track, and
			// in case record its value
			if(pack_states[j].found_note_state == 0)
				{
				pack_states[j].found_note_state = 1;
				pack_states[j].first_note_found = pat2->note;
				}
			// if we find more than 1 note in a track, we need to match
			// it against the 1st note.
			else if(pack_states[j].found_note_state == 1 &&
				pack_states[j].first_note_found != pat2->note)
				pack_states[j].found_note_state = 2;
			// check instruments also
			if(pack_states[j].found_instrument_state == 0)
				{
				pack_states[j].found_instrument_state = 1;
				pack_states[j].first_instrument_found = pat2->instrument;
				}
			else if(pack_states[j].found_instrument_state == 1 &&
				pack_states[j].first_instrument_found != pat2->instrument)
				pack_states[j].found_instrument_state = 2;
			// check if we have volume column effects
			if(pat2->vol_command != 0)
				pack_states[j].found_volume_command = 1:
			if(pat2->effect_command != 0 || pat2->effect_byte != 0)
				pack_states[j].found_effect_command = 1;
			}
		}
	// Then: prepend the track state flags and non-empty bitmap
	// to the beginning of the pattern structure.
	u8 *packed_pointer = packed_data_area;
	for(i=0;i<channels;i++)
		{
		u8 flags = 0;
		if(pack_states[i].found_note_state == 1)
			flags |= 1;
		if(pack_states[i].found_instrument_state == 1)
			flags |= 2;
		if(pack_states[i].found_volume_command)
			flags |= 4;
		if(pack_states[i].found_effect_command)
			flags |= 8;
		*packed_pointer++ = flags;
		if(flags & 1)
			*packed_pointer++ = pack_states[i].first_note_found;
		if(flags & 2)
			*packed_pointer++ = pack_states[i].first_instrument_found;
		}
	for(i=0;i<rows;i++)
		if(nonempty_row_bitmap[i])
			packed_pointer[i >> 3] |= 1 << (i & 7);
	packed_pointer += (rows+7) >> 3;

	// then, start to pack actual pattern data
	for(i=0; i<rows; i++)
		{
		u8 *notebitmap_ptr = packed_pointer;
		packed_pointer += (channels + 7) >> 3;
		for(j=0; j<channels; j++)
			{
			pattern_note *pat2 = pattern + i*channels + j;
			// if the pattern entry is all 0s, we ahve an empty note. Don't
			// bother processing it.
			if(pat2->note == 0
				&& pat2->instrument == 0
				&& pat2->vol_command == 0
				&& pat2->effect_command == 0
				&& pat2->effect_byte == 0)
				continue;
			// flag the note as not-empty in the row's bitmap
			notebitmap_ptr[j >> 3] |= 1 << (j & 7);
			// for each byte of the note, use pack-state for the track
			// to determine if we shall write it.
			if(pack_states[i].found_note_state != 1)
				*packed_pointer++ = pat2->note;
			if(pack_states[i].found_instrument_state != 1)
				*packed_pointer++ = pat2->instrument;
			if(pack_states[i].found_volume_command)
				*packed_pointer++ = pat2->vol_command;
			if(pack_states[i].found_effect_command)
				{
				u8 effect_cmd = pat2->effect_command;
				if(pat2->effect_byte)
					effect_cmd |= 0x80;
				*packed_pointer++ = effect_cmd;
				if(pat2->effect_byte)
					*packed_pointer++ = pat2->effect_byte;
				}
			}
		}
	// finally shrink the memory buffer and return it to the
	// application that called it.
	int actual_length = packed_pointer - packed_data_area;
	*packed_data = realloc( packed_data_area, actual_length );
	*length = actual_length;
	};



void generate_row_pointers(
	int *order, // module play-order, terminated with -1
	int repstart, // module rep-start location

)






