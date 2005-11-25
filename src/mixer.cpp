#include <stdlib.h>
#include <assert.h>
#include "mixer.h"

#include <gba_systemcalls.h>
#include <gba_video.h>
#include <gba_dma.h>
#include <gba_timers.h>

using namespace mixer;

/*

miksing:

for each chan
	if looping && loop event (sample-stop is loop event)
		event_cursor = event_point;
		do {
			do {
				mix sample
				event_cursor -= event_delta
			}
			while (event_cursor > 0)
			
			handle event
		}
		while (not all samples rendered)
		
		mix all samples in tick with loop-check
	else
		if (volume == max) mix_megafast()
		mix all samples (unroll, baby)
	end if
end for

*/

volatile channel_t mixer::channels[CHANNELS] IWRAM_DATA;

#include <gba_console.h>
#include <stdio.h>

s32 event_delta = 0;
s32 event_cursor = 0;

bool detect_loop_event(channel_t &chan, size_t samples)
{
	assert(samples != 0);
	
	s32 check_point;
	s32 end_sample = chan.sample_cursor + chan.sample_cursor_delta * samples;
	
	switch (chan.sample->loop_type)
	{
	case LOOP_TYPE_NONE:
		check_point = chan.sample->len;
	break;
	case LOOP_TYPE_FORWARD:
		check_point = chan.sample->loop_end;
	break;
	case LOOP_TYPE_PINGPONG:
		if (chan.sample_cursor_delta >= 0)
		{
			// moving forwards through the sample
			check_point = chan.sample->loop_end;
		}
		else
		{
			// moving backwards through the sample
			check_point = chan.sample->loop_start;
			if (end_sample < (check_point << 12))
			{
				event_delta = -chan.sample_cursor_delta;
				event_cursor = -((check_point << 12) - chan.sample_cursor);
				return true;
			}
			else return false;
		}
	break;
	}
	
	if (end_sample >= (check_point << 12))
	{
		event_delta = chan.sample_cursor_delta;
		event_cursor = (check_point << 12) - chan.sample_cursor;
		return true;
	}
	
	return false;
}

// returns false if we hit sample-end
bool process_loop_event(channel_t &chan)
{
	switch (chan.sample->loop_type)
	{
	case LOOP_TYPE_NONE:
		return false;
	break;
	case LOOP_TYPE_FORWARD:
		do
		{
			chan.sample_cursor -= (chan.sample->loop_end - chan.sample->loop_start) << 12;
		}
		while (chan.sample_cursor >= (chan.sample->loop_end << 12));
	break;
	case LOOP_TYPE_PINGPONG:
		do
		{
			if (chan.sample_cursor_delta >= 0)
			{
				chan.sample_cursor -= chan.sample->loop_end << 12;
				chan.sample_cursor = -chan.sample_cursor;
				chan.sample_cursor += chan.sample->loop_end << 12;
			}
			else
			{
				chan.sample_cursor -= chan.sample->loop_start << 12;
				chan.sample_cursor = -chan.sample_cursor;
				chan.sample_cursor += chan.sample->loop_start << 12;
			}
			chan.sample_cursor_delta = -chan.sample_cursor_delta;
		}
		while (chan.sample_cursor > (chan.sample->loop_end << 12) || chan.sample_cursor < (chan.sample->loop_start << 12));
	break;
	}

	return true;
}

inline void timing_start()
{
	REG_TM3CNT_H = 0;
	REG_TM3CNT_L = 0;
	REG_TM3CNT_H = TIMER_START;
}

inline void timing_end()
{
	unsigned int fjall = REG_TM3CNT_L;
//	iprintf("cycles pr sample: %i\n", fjall / SOUND_BUFFER_SIZE);
//	iprintf("%i per cent cpu\n", (fjall * 1000) / 280896);
}

static u32 mix_simple(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
	asm(
"\
	b .dataskip                             \n\
.stack_store:                               \n\
.align 4                                    \n\
.dataskip:                                  \n\
	str sp, .stack_store                    \n\
.loop:                                      \n\
	ldmia %[target], {r0-r7}                \n\
	                                        \n\
	ldrb  sp, [%[data], %[cursor], lsr #12] \n\
	mla   r0, sp, %[vol], r0                \n\
	add   %[cursor], %[cursor], %[delta]    \n\
	                                        \n\
	ldrb  sp, [%[data], %[cursor], lsr #12] \n\
	mla   r1, sp, %[vol], r1                \n\
	add   %[cursor], %[cursor], %[delta]    \n\
	                                        \n\
	ldrb  sp, [%[data], %[cursor], lsr #12] \n\
	mla   r2, sp, %[vol], r2                \n\
	add   %[cursor], %[cursor], %[delta]    \n\
	                                        \n\
	ldrb  sp, [%[data], %[cursor], lsr #12] \n\
	mla   r3, sp, %[vol], r3                \n\
	add   %[cursor], %[cursor], %[delta]    \n\
	                                        \n\
	ldrb  sp, [%[data], %[cursor], lsr #12] \n\
	mla   r4, sp, %[vol], r4                \n\
	add   %[cursor], %[cursor], %[delta]    \n\
	                                        \n\
	ldrb  sp, [%[data], %[cursor], lsr #12] \n\
	mla   r5, sp, %[vol], r5                \n\
	add   %[cursor], %[cursor], %[delta]    \n\
	                                        \n\
	ldrb  sp, [%[data], %[cursor], lsr #12] \n\
	mla   r6, sp, %[vol], r6                \n\
	add   %[cursor], %[cursor], %[delta]    \n\
	                                        \n\
	ldrb  sp, [%[data], %[cursor], lsr #12] \n\
	mla   r7, sp, %[vol], r7                \n\
	add   %[cursor], %[cursor], %[delta]    \n\
	                                        \n\
	stmia %[target]!, {r0-r7}               \n\
	subs  %[counter], %[counter], #1        \n\
	bne .loop                               \n\
	ldr sp, .stack_store                    \n\
"
	: "=r"(sample_cursor)
	:
		[cursor]  "0"(sample_cursor),
		[counter] "r"(samples >> 3),
		[data]    "r"(sample_data),
		[target]  "r"(target),
		[delta]   "r"(sample_cursor_delta),
		[vol]     "r"(vol)
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "sp", "1", "2", "4", "cc"
	);
	return sample_cursor;
}

static u32 mix_bresenham(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
	const u8 *old_sample_data = sample_data;
	sample_data += (sample_cursor >> 12);
	asm(
"\
	b .dataskip2                            \n\
.stack_store2:                              \n\
.align 4                                    \n\
.word                                       \n\
.dataskip2:                                 \n\
	str sp, .stack_store2                   \n\
	ldrb  sp, [%[data]], #1                 \n\
	mul   sp, %[vol], sp                    \n\
.loop2:                                     \n\
	ldmia %[target], {r0-r7}                \n\
	                                        \n\
	add   r0, sp, r0                        \n\
	adds  %[cursor], %[cursor], %[delta]    \n\
	ldrcsb sp, [%[data]], #1                \n\
	mulcs  sp, %[vol], sp                   \n\
	                                        \n\
	add   r1, sp, r1                        \n\
	adds  %[cursor], %[cursor], %[delta]    \n\
	ldrcsb sp, [%[data]], #1                \n\
	mulcs  sp, %[vol], sp                   \n\
	                                        \n\
	add   r2, sp, r2                        \n\
	adds  %[cursor], %[cursor], %[delta]    \n\
	ldrcsb sp, [%[data]], #1                \n\
	mulcs  sp, %[vol], sp                   \n\
	                                        \n\
	add   r3, sp, r3                        \n\
	adds  %[cursor], %[cursor], %[delta]    \n\
	ldrcsb sp, [%[data]], #1                \n\
	mulcs  sp, %[vol], sp                   \n\
	                                        \n\
	add   r4, sp, r4                        \n\
	adds  %[cursor], %[cursor], %[delta]    \n\
	ldrcsb sp, [%[data]], #1                \n\
	mulcs  sp, %[vol], sp                   \n\
	                                        \n\
	add   r5, sp, r5                        \n\
	adds  %[cursor], %[cursor], %[delta]    \n\
	ldrcsb sp, [%[data]], #1                \n\
	mulcs  sp, %[vol], sp                   \n\
	                                        \n\
	add   r6, sp, r6                        \n\
	adds  %[cursor], %[cursor], %[delta]    \n\
	ldrcsb sp, [%[data]], #1                \n\
	mulcs  sp, %[vol], sp                   \n\
	                                        \n\
	add   r7, sp, r7                        \n\
	adds  %[cursor], %[cursor], %[delta]    \n\
	ldrcsb sp, [%[data]], #1                \n\
	mulcs  sp, %[vol], sp                   \n\
	                                        \n\
	stmia %[target]!, {r0-r7}               \n\
	subs  %[counter], %[counter], #1        \n\
	bne .loop2                              \n\
	ldr sp, .stack_store2                   \n\
"
	: "=r"(sample_cursor), "=r"(sample_data)
	:
		[cursor]  "0"(sample_cursor << 20),
		[counter] "r"(samples >> 3),
		[data]    "1"(sample_data),
		[target]  "r"(target),
		[delta]   "r"(sample_cursor_delta << 20),
		[vol]     "r"(vol)
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "sp", "cc"
	);
	return ((sample_data - old_sample_data) << 12) + (sample_cursor >> 20);
}

u32 dc_offs = 0;
static inline void mix_channel(channel_t &chan, s32 *target, size_t samples)
{
	dc_offs += chan.volume * 128;

	assert(samples > 0);
	while (samples > 0 && detect_loop_event(chan, samples) == true)
	{
		BG_COLORS[0] = RGB5(0, 31, 0);
		do
		{
			assert((chan.sample_cursor >> 12) < chan.sample->len);
			
			s32 samp = ((u8*)chan.sample->data)[chan.sample_cursor >> 12];
			chan.sample_cursor += chan.sample_cursor_delta;
			*target++ += samp * chan.volume;
			
			samples--;
			event_cursor -= event_delta;
		} while (event_cursor > 0);
		
		assert(samples >= 0);
		
		if (process_loop_event(chan) == false)
		{
			while (samples--)
			{
				*target++ += chan.volume * 128;
			}
			
			chan.sample = 0;
			return;
		}
	}
	
	BG_COLORS[0] = RGB5(31, 0, 31);
	
	const u8 *const sample_data = (const u8 *const)chan.sample->data;
	u32 sample_cursor = chan.sample_cursor;
	const u32 sample_cursor_delta = chan.sample_cursor_delta;
	const u32 vol = chan.volume;
	
	/*
		ldrb	r3, [sl, r4, lsr #12]	@ zero_extendqisi2	@ samp,* sample_data
		ldr	r2, [r5, #0]	@ tmp348,* target
		mla	r1, r3, r8, r2	@ tmp349, samp, <variable>.volume, tmp348
		str	r1, [r5], #4	@ tmp349,
		add	r4, r4, r6	@ sample_cursor, sample_cursor, <variable>.sample_cursor_delta
		
		LDRB: 1S + 1N + 1I <- 5 cycles?! (wait state, vettu) (men, to cycles...?)
		LDR : 1S + 1N + 1I <- 3 cycles
		MLA : 1S + 1I + 1I <- multiply-delen koster 2 cycles over ADD, accumulate-delen koster 1 cycle over MUL
		STR : 2N
		ADD : 1S
	*/

	/* TODO: non-duffs device "real" unrolling with LDM/STM. should save us 4 cycles, and get this loop down to 10 cycles (yay) */

#if 1

	/*
	12 cycles / sample. TODO: manage to squeeze in 8 (4 more) samples each load/store.
	we seem to only be one register short, so disabling the stack-pointer might work.
	
	hmm, need an absolute adressed temp-storage for sp... ?	
	*/

	/*
	update: just fixed it. 11 cycles. i think this might be the best we'll get with the wuline approach.
	(with the exception of the dataskip-branch, but that's a low priority issue as it's a constant
	overhead pr channel of only a few cycles. it might be an issue with short frame-lengths...)
	*/
	timing_start();

//	chan.sample_cursor = mix_bresenham(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);
//	chan.sample_cursor = mix_simple(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);

	if (sample_cursor_delta > 0 && sample_cursor_delta < u32((1 << 12) * 0.95))
	{
		BG_COLORS[0] = RGB5(0, 0, 31);
		chan.sample_cursor = mix_bresenham(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);
	}
	else
	{
		BG_COLORS[0] = RGB5(31, 0, 0);
		chan.sample_cursor = mix_simple(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);
	}

	timing_end();

#else

	timing_start();
	register u32 s = samples >> 4;
	switch (samples & 15)
	{
		do
		{
#define ITERATION                             \
	{                                         \
		register s32 samp = sample_data[sample_cursor >> 12]; \
		sample_cursor += sample_cursor_delta; \
		*target++ += samp * vol;              \
	}
		ITERATION;
		case 15: ITERATION;
		case 14: ITERATION;
		case 13: ITERATION;
		case 12: ITERATION;
		case 11: ITERATION;
		case 10: ITERATION;
		case 9:  ITERATION;
		case 8:  ITERATION;
		case 7:  ITERATION;
		case 6:  ITERATION;
		case 5:  ITERATION;
		case 4:  ITERATION;
		case 3:  ITERATION;
		case 2:  ITERATION;
		case 1:  ITERATION;
#undef ITERATION
		case 0:;
		}
		while (s--);
	}
	timing_end();
#endif
	BG_COLORS[0] = RGB5(31, 0, 0);
}

void mixer::reset()
{
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		channels[c].sample = 0;
		channels[c].sample_cursor = 0;
		channels[c].sample_cursor_delta = 0;
	}
}

s32 sound_mix_buffer[SOUND_BUFFER_SIZE] IWRAM_DATA ALIGN(4);

void mixer::mix(s8 *target, size_t samples)
{
	assert(samples > 0);
	
	BG_COLORS[0] = RGB5(31, 0, 0);
	
	u32 zero = 0;
	CpuFastSet(&zero, sound_mix_buffer, DMA_SRC_FIXED | (samples));
	
	
	/* these comments are for 16bit mixing, we're currently doing it in 32bit */
	// CpuFastSet works on groups of 4 bytes, so the last sample might not be cleared.
	// unconditional clear is faster, so let's do it anyway
//	u32 zero = 0;
//	CpuFastSet(&zero, sound_mix_buffer, DMA_SRC_FIXED | (samples));
//	sound_mix_buffer[samples - 1] = 0;

	dc_offs = 0;
	for (u32 c = 0; c < CHANNELS; ++c)
	{
		channel_t &chan = (channel_t &)channels[c];
		if (0 != chan.sample) mix_channel(chan, sound_mix_buffer, samples);
	}
	dc_offs >>= 8;

	BG_COLORS[0] = RGB5(0, 0, 31);
	
	register s32 *src = sound_mix_buffer;
	register s8  *dst = target;
	register u32 dc_offs_local = dc_offs;
	
	// the compiler is too smart -- we need to prevent it from doing some arm11-optimizations.
	register s32 high_clamp = 127 + dc_offs;
	register s32 low_clamp = -128 + dc_offs;
	
#define ITERATION                                 \
	{	                                          \
		s32 samp = (*src++) >> 8;                 \
		if (samp > high_clamp) samp = high_clamp; \
		if (samp < low_clamp) samp = low_clamp;   \
		samp -= dc_offs_local;                    \
		*dst++ = samp;                            \
	}
	
	register u32 s = samples >> 4;
	switch (samples & 15)
	{
		do
		{
		ITERATION;
		case 15: ITERATION;
		case 14: ITERATION;
		case 13: ITERATION;
		case 12: ITERATION;
		case 11: ITERATION;
		case 10: ITERATION;
		case 9:  ITERATION;
		case 8:  ITERATION;
		case 7:  ITERATION;
		case 6:  ITERATION;
		case 5:  ITERATION;
		case 4:  ITERATION;
		case 3:  ITERATION;
		case 2:  ITERATION;
		case 1:  ITERATION;
		case 0:;
		}
		while (s--);
	}
#undef ITERATION

	BG_COLORS[0] = RGB5(0, 0, 0);
}
