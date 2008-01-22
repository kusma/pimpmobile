#include "../framework/test_framework.h"
#include "../framework/test_helpers.h"

#include "src/pimp_mixer.h"
#include <string.h>
#include <stdlib.h>

/*

typedef struct
{
	u32                   sample_length;
	u32                   loop_start;
	u32                   loop_end;
	pimp_mixer_loop_type  loop_type;
	const u8             *sample_data;
	u32                   sample_cursor;
	s32                   sample_cursor_delta;
	u32                   event_cursor;
	s32                   volume;
} pimp_mixer_channel_state;

typedef struct
{
	pimp_mixer_channel_state channels[CHANNELS];
	s32 *mix_buffer;
} pimp_mixer;

void pimp_mixer_mix(pimp_mixer *mixer, s8 *target, int samples);
*/


#define MAX_TARGET_SIZE 1024
s8  target[MAX_TARGET_SIZE + 2];
s32 mix_buffer[MAX_TARGET_SIZE + 2];

static void test_mixer_basic(struct test_suite *suite)
{
	/*
		cases that need testing:
			- that mixer never writes outside mix- and target buffers
			- that odd mix-buffer sizes does not write outside buffer
	*/
	int target_size;
	
	pimp_mixer mixer;
	pimp_mixer_reset(&mixer);
	mixer.mix_buffer = mix_buffer + 1;
	
	/* try all buffer sizes */	
	for (target_size = 0; target_size < MAX_TARGET_SIZE; ++target_size)
	{
		s8 rnd = rand();
		
		target[0] = rnd;
		target[target_size + 1] = rnd;
		mix_buffer[0] = rnd;
		mix_buffer[target_size + 1] = rnd;
		
		pimp_mixer_mix(&mixer, target + 1, target_size);
		
		/* test that values outside the buffer haven't been written */
		/* target */
		ASSERT_INTS_EQUAL(suite, target[0], rnd);
		ASSERT_INTS_EQUAL(suite, target[target_size + 1], rnd);	
		/* mix_buffer */
		ASSERT_INTS_EQUAL(suite, mix_buffer[target_size + 1], rnd);	
		ASSERT_INTS_EQUAL(suite, mix_buffer[0], rnd);
	}
}

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

int pimp_linear_search_loop_event(int event_cursor, int event_delta, int max_samples);
int pimp_calc_loop_event(int event_cursor, int event_delta, int max_samples);

/* reference implementation */
STATIC PURE int ref_search_loop_event(int event_cursor, int event_delta, const int max_samples)
{
	int i;
	for (i = 0; i < max_samples; ++i)
	{
		event_cursor -= event_delta;
		if (event_cursor <= 0)
		{
			return i + 1;
		}
	}
	return -1;
}

static void test_looping(struct test_suite *suite)
{
	/*
	cases that need testing:
		- forward looping
		- ping pong looping
		- that a loop never reads outside of the start and stop points
		- that a loop happens at the correct sub-sample
	*/

	pimp_mixer_channel_state chan;
	
	const u8 sample_data[] = { 0x00, 0x01, 0x02, 0x03, 0x04 };
	
	/* quick, let's test pimp_mixer_detect_loop_event() */
	{
		int i;
		
		for (i = 0; i < 1000000; ++i)
		{
			int max_samples = rand() % 1024;
			int event_cursor = abs(rand() * rand());
			int event_delta = abs(rand() * rand()) % (1 << 19);
			int correct = ref_search_loop_event(event_cursor, event_delta, max_samples);
			int res = pimp_calc_loop_event(event_cursor, event_delta, max_samples);
			ASSERT_INTS_EQUAL(suite, res, correct);
		}
		
		chan.loop_type           = LOOP_TYPE_FORWARD;
		chan.loop_start          = 0;
		chan.loop_end            = 4;
		chan.sample_cursor       = 0 << 12;
		chan.sample_cursor_delta = 1 << 12;

		/* test that the clamp to buffer size happens at the right place */
		ASSERT_INTS_EQUAL(suite, pimp_mixer_detect_loop_event(&chan, 1), -1);
		ASSERT_INTS_EQUAL(suite, pimp_mixer_detect_loop_event(&chan, 2), -1);
		ASSERT_INTS_EQUAL(suite, pimp_mixer_detect_loop_event(&chan, 3), -1);
		ASSERT_INTS_EQUAL(suite, pimp_mixer_detect_loop_event(&chan, 4), 4);
		ASSERT_INTS_EQUAL(suite, pimp_mixer_detect_loop_event(&chan, 5), 4);
		
		/* see that the correct amount of samples are mixed event at the border values -- both these locations are inside the same sample */
		chan.sample_cursor       = (0 << 12) + 1;
		ASSERT_INTS_EQUAL(suite, pimp_mixer_detect_loop_event(&chan, 5), 4);
		chan.sample_cursor       = (1 << 12) - 1;
		ASSERT_INTS_EQUAL(suite, pimp_mixer_detect_loop_event(&chan, 5), 4);
	}

	
	chan.sample_length       = ARRAY_SIZE(sample_data);
	chan.loop_start          = 0;
	chan.loop_end            = 4;
	chan.loop_type           = LOOP_TYPE_FORWARD;
	chan.sample_data         = sample_data;
	chan.sample_cursor       = 0 << 12;
	chan.sample_cursor_delta = 1 << 12;
	chan.volume              = 1;
	
	{
		const s32 forward_loop_ref[] = {
			0x00, 0x01, 0x02, 0x03, /* loop */
			0x00, 0x01, 0x02, 0x03 /* done */
		};
		
		int target_size = 8;
	
		/* loop should happen exactly at loop-end */
		chan.sample_cursor       = 0 << 12;
		chan.sample_cursor_delta = 1 << 12;
		memset(mix_buffer, 0, target_size * sizeof(u32));
		pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
		ASSERT_INT_ARRAYS_EQUAL(suite, mix_buffer, forward_loop_ref, ARRAY_SIZE(forward_loop_ref));
	
		/* loop should happen right after loop-end */
		chan.sample_cursor       = (0 << 12) + 1;
		chan.sample_cursor_delta = 1 << 12;
		memset(mix_buffer, 0, target_size * sizeof(u32));
		pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
		ASSERT_INT_ARRAYS_EQUAL(suite, mix_buffer, forward_loop_ref, ARRAY_SIZE(forward_loop_ref));
	
		/* loop should happen way after loop-end */
		chan.sample_cursor       = (1 << 12) - 1;
		chan.sample_cursor_delta = 1 << 12;
		memset(mix_buffer, 0, target_size * sizeof(u32));
		pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
		ASSERT_INT_ARRAYS_EQUAL(suite, mix_buffer, forward_loop_ref, ARRAY_SIZE(forward_loop_ref));
	}

	{
		const s32 pingpong_loop_ref[] = {
			0x00, 0x01, 0x02, 0x03, /* change direction */
			0x03, 0x02, 0x01, 0x00, /* change direction */
			0x00, 0x01, 0x02, 0x03  /* done */ };	
		int target_size = 8 + 4;
		
		/* loop should happen exactly at loop-end */
		chan.loop_type           = LOOP_TYPE_PINGPONG;
		chan.sample_cursor       = 0 << 12;
		chan.sample_cursor_delta = 1 << 12;
		memset(mix_buffer, 0, target_size * sizeof(u32));
		pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
		ASSERT_INT_ARRAYS_EQUAL(suite, mix_buffer, pingpong_loop_ref, ARRAY_SIZE(pingpong_loop_ref));
	
		/* loop should happen right after loop-end */
		chan.loop_type           = LOOP_TYPE_PINGPONG;
		chan.sample_cursor       = (0 << 12) + 1;
		chan.sample_cursor_delta = 1 << 12;
		memset(mix_buffer, 0, target_size * sizeof(u32));
		pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
		ASSERT_INT_ARRAYS_EQUAL(suite, mix_buffer, pingpong_loop_ref, ARRAY_SIZE(pingpong_loop_ref));
	
		/* loop should happen way after loop-end */
		chan.loop_type           = LOOP_TYPE_PINGPONG;
		chan.sample_cursor       = (1 << 12) - 1;
		chan.sample_cursor_delta = 1 << 12;
		memset(mix_buffer, 0, target_size * sizeof(u32));
		pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
		ASSERT_INT_ARRAYS_EQUAL(suite, mix_buffer, pingpong_loop_ref, ARRAY_SIZE(pingpong_loop_ref));
	}
}

void test_mixer(struct test_suite *suite)
{
	test_mixer_basic(suite);
	test_looping(suite);
}
