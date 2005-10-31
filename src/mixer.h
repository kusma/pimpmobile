#ifndef _MIXER_H_
#define _MIXER_H_

#include <gba_base.h>

#define CHANNELS 32
#define SOUND_BUFFER_SIZE 304

#define UNSIGNED_SAMPLES

namespace mixer
{

typedef enum
{
	LOOP_TYPE_NONE,
	LOOP_TYPE_FORWARD,
	LOOP_TYPE_PINGPONG
} loop_type_t;

typedef struct sample_
{
	u32         len;
	u32         loop_start;
	u32         loop_end;
	loop_type_t loop_type;
#ifdef UNSIGNED_SAMPLES
	const u8   *data;
#else
	const s8   *data;
#endif
} sample_t;

typedef struct channel_state_
{
	sample_t *sample;
	u32       sample_cursor;
	u32       sample_cursor_delta;
	u32       volume;
} channel_t;

extern channel_t channels[CHANNELS];

void reset();
void mix(s8 *target, size_t samples);

}

#endif /* _MIXER_H_ */
