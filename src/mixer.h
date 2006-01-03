#ifndef MIXER_H
#define MIXER_H

#include <gba_base.h>
#include "config.h"

namespace mixer
{

	typedef enum
	{
		LOOP_TYPE_NONE,
		LOOP_TYPE_FORWARD,
		LOOP_TYPE_PINGPONG
	} loop_type_t;
	
	typedef struct channel_state_
	{
		u32         sample_length;
		u32         loop_start;
		u32         loop_end;
		loop_type_t loop_type;
		const u8   *sample_data;
		u32         sample_cursor;
		s32         sample_cursor_delta;
		u32         volume;
	} channel_t;
	
	extern volatile channel_t channels[CHANNELS];
	
	void reset();
	void mix(s8 *target, size_t samples);
	u32 mix_samples(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta);

}

#endif /* MIXER_H */
