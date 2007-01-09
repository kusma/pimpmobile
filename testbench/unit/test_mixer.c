#include <src/pimp_mixer.h>
#include "../framework/test.h"
#include "../framework/helpers.h"

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

void __pimp_mixer_mix(pimp_mixer *mixer, s8 *target, int samples);
*/


#define MAX_TARGET_SIZE 1024
s8  target[MAX_TARGET_SIZE + 2];
s32 mix_buffer[MAX_TARGET_SIZE + 2];

static void test_mixer_basic(void)
{
	/*
		cases that need testing:
			- that mixer never writes outside mix- and target buffers
			- that odd mix-buffer sizes does not write outside buffer
	*/
	int target_size;
	
	pimp_mixer mixer;
	__pimp_mixer_reset(&mixer);
	mixer.mix_buffer = mix_buffer + 1;
	
	/* try all buffer sizes */	
	for (target_size = 0; target_size < MAX_TARGET_SIZE; ++target_size)
	{
		s8 rnd = rand();
		
		target[0] = rnd;
		target[target_size + 1] = rnd;
		mix_buffer[0] = rnd;
		mix_buffer[target_size + 1] = rnd;
		
		__pimp_mixer_mix(&mixer, target + 1, target_size);
		
		/* test that values outside the buffer haven't been written */
		/* target */
		TEST_INTS_EQUAL(target[0], rnd);
		TEST_INTS_EQUAL(target[target_size + 1], rnd);	
		/* mix_buffer */
		TEST_INTS_EQUAL(mix_buffer[target_size + 1], rnd);	
		TEST_INTS_EQUAL(mix_buffer[0], rnd);
	}
}

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

int linear_search_loop_event(int event_cursor, int event_delta, int max_samples);
int calc_loop_event(int event_cursor, int event_delta, int max_samples);

static void test_looping(void)
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
			int correct = linear_search_loop_event(event_cursor, event_delta, max_samples);
			int res = calc_loop_event(event_cursor, event_delta, max_samples);
			TEST_INTS_EQUAL(res, correct);
		}
		
		chan.loop_type           = LOOP_TYPE_FORWARD;
		chan.loop_start          = 0;
		chan.loop_end            = 4;
		chan.sample_cursor       = 0 << 12;
		chan.sample_cursor_delta = 1 << 12;

		/* test that the clamp to buffer size happens at the right place */
		TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, 1), -1);
		TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, 2), -1);
		TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, 3), -1);
		TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, 4), 4);
		TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, 5), 4);
#if 0
#define CLAMP 4
		for (i = 1; i < CLAMP; ++i)
		{
			TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, i), i);
		}
		
		for (i = CLAMP; i < 1024; ++i)
		{
			TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, i), CLAMP);
			TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, i), CLAMP);
		}
#endif
		/* see that the correct amount of samples are mixed event at the border values */
		chan.sample_cursor       = (0 << 12) + 1;
		TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, 5), 4);
		chan.sample_cursor       = (1 << 12) - 1;
		TEST_INTS_EQUAL(pimp_mixer_detect_loop_event(&chan, 5), 4);
	}

	
	chan.sample_length       = ARRAY_SIZE(sample_data);
	chan.loop_start          = 0;
	chan.loop_end            = 4;
	chan.loop_type           = LOOP_TYPE_FORWARD;
	chan.sample_data         = sample_data;
	chan.sample_cursor       = 0 << 12;
	chan.sample_cursor_delta = 1 << 12;
	chan.volume              = 1;
	
	const s32 forward_loop_ref[] = {
		0x00, 0x01, 0x02, 0x03, /* loop */
		0x00, 0x01, 0x02, 0x03 /* done */
	};
	
	int target_size = 8;

	/* loop should happen exactly at loop-end */
	chan.sample_cursor       = 0 << 12;
	chan.sample_cursor_delta = 1 << 12;
	memset(mix_buffer, 0, target_size * sizeof(u32));
	__pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
	TEST_INT_ARRAYS_EQUAL(mix_buffer, forward_loop_ref, ARRAY_SIZE(forward_loop_ref));

	/* loop should happen right after loop-end */
	chan.sample_cursor       = (0 << 12) + 1;
	chan.sample_cursor_delta = 1 << 12;
	memset(mix_buffer, 0, target_size * sizeof(u32));
	__pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
	TEST_INT_ARRAYS_EQUAL(mix_buffer, forward_loop_ref, ARRAY_SIZE(forward_loop_ref));

	/* loop should happen way after loop-end */
	chan.sample_cursor       = (1 << 12) - 1;
	chan.sample_cursor_delta = 1 << 12;
	memset(mix_buffer, 0, target_size * sizeof(u32));
	__pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
	TEST_INT_ARRAYS_EQUAL(mix_buffer, forward_loop_ref, ARRAY_SIZE(forward_loop_ref));

	const s32 pingpong_loop_ref[] = {
		0x00, 0x01, 0x02, 0x03, /* change direction */
		0x03, 0x02, 0x01, 0x00, /* change direction */
		0x00, 0x01, 0x02, 0x03  /* done */ };	
	target_size = 8 + 4;
	
	/* loop should happen exactly at loop-end */
	chan.loop_type           = LOOP_TYPE_PINGPONG;
	chan.sample_cursor       = 0 << 12;
	chan.sample_cursor_delta = 1 << 12;
	memset(mix_buffer, 0, target_size * sizeof(u32));
	__pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
	TEST_INT_ARRAYS_EQUAL(mix_buffer, pingpong_loop_ref, ARRAY_SIZE(pingpong_loop_ref));

	/* loop should happen right after loop-end */
	chan.loop_type           = LOOP_TYPE_PINGPONG;
	chan.sample_cursor       = (0 << 12) + 1;
	chan.sample_cursor_delta = 1 << 12;
	memset(mix_buffer, 0, target_size * sizeof(u32));
	__pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
	TEST_INT_ARRAYS_EQUAL(mix_buffer, pingpong_loop_ref, ARRAY_SIZE(pingpong_loop_ref));

	/* loop should happen way after loop-end */
	chan.loop_type           = LOOP_TYPE_PINGPONG;
	chan.sample_cursor       = (1 << 12) - 1;
	chan.sample_cursor_delta = 1 << 12;
	memset(mix_buffer, 0, target_size * sizeof(u32));
	__pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
	TEST_INT_ARRAYS_EQUAL(mix_buffer, pingpong_loop_ref, ARRAY_SIZE(pingpong_loop_ref));
}

void test_mixer(void)
{
	test_mixer_basic();
	test_looping();
}
