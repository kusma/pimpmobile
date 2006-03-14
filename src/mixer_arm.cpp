#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "mixer.h"
#include "debug.h"

#include <gba_base.h>
#include <gba_video.h>
#include <gba_interrupt.h>

/*
	IDEA:
	avoiding buffer clearing: self modifying code...
	for the first channel mixed:
		- replace the ldmia with a nop
		- set bit #1 in byte #3 of the MLAs to 0 to make them MULs instead
		- then for the next chan, set all stuff back
	this way we don't have to clear the sample-buffer before mixing
	to it, and we've saved some cycles on the first channel.
	the advantage over having separate loops is less iwram-usage.
*/

/*

the magic bug: is it caused by interrupting while not having a stack-pointer set up? if so, try to disable interrupts while mixing...

helped on something, but not on all...

*/

static u32 mix_simple(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
	assert(target != NULL);
	assert(sample_data != NULL);
	assert((samples & 7) == 0);
	assert(samples != 0);
	
	/* disable interrupts */
	u32 ime = REG_IME;
	REG_IME = 0;
	
/*
	ADD              1S
	OR               1S
	OR + shift       1S+1I
	MUL              1S+mI
	MLA              1S+(m+1)I
*/
//	iprintf("%d...", samples);
	asm(
"\
	b .Ldataskip%=                          \n\
.Lstack_store%=:                            \n\
.word 0                                     \n\
.Ldataskip%=:                               \n\
	str sp, .Lstack_store%=                 \n\
.Lloop%=:                                   \n\
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
	bne .Lloop%=                            \n\
	ldr sp, .Lstack_store%=                 \n\
"
	:   "=r"(sample_cursor)
	:
		[cursor]  "0"(sample_cursor),
		[counter] "r"(samples / 8),
		[data]    "r"(sample_data),
		[target]  "r"(target),
		[delta]   "r"(sample_cursor_delta),
		[vol]     "r"(vol)
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "sp", "1", "2", "4", "cc"
	);

	/* restore interrupt state */
	REG_IME = ime;
	
	return sample_cursor;
}

/* bugs here sometimes for some strange reason... ?? (magic bug2k?) */
static u32 mix_bresenham(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
	const u8 *old_sample_data = sample_data;
	sample_data += (sample_cursor >> 12);

	assert(target != NULL);
	assert(sample_data != NULL);
	assert((samples & 7) == 0);
	assert(samples != 0);

	/* disable interrupts */
	u32 ime = REG_IME;
	REG_IME = 0;

	asm(
"\
	b .Ldataskip%=                          \n\
.Lstack_store%=:                            \n\
.word 0                                     \n\
.Ldataskip%=:                               \n\
	str sp, .Lstack_store%=                 \n\
	ldrb  sp, [%[data]], #1                 \n\
	mul   sp, %[vol], sp                    \n\
.Lloop%=:                                   \n\
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
	bne .Lloop%=                            \n\
	ldr sp, .Lstack_store%=                 \n\
"
	: "=r"(sample_cursor), "=r"(sample_data)
	:
		[cursor]  "0"(sample_cursor << 20),
		[counter] "r"(samples / 8),
		[data]    "1"(sample_data),
		[target]  "r"(target),
		[delta]   "r"(sample_cursor_delta << 20),
		[vol]     "r"(vol)
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "sp", "cc"
	);

	/* restore interrupt state */
	REG_IME = ime;
	
	return ((sample_data - old_sample_data - 1) << 12) + (sample_cursor >> 20);
}

u32 mixer::mix_samples(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
//	PROFILE_COLOR(31, 0, 31);
	assert(target != 0);
	assert(sample_data != 0);
	
	/* mix heading 0-7 samples (the innerloops are unrolled 8 times) */
	for (unsigned i = samples & 7; i; --i)
	{
		register s32 samp = sample_data[sample_cursor >> 12];
		sample_cursor += sample_cursor_delta;
		*target++ += samp * vol;
		samples--;
	}

	if (samples == 0) return sample_cursor;

	/* decide what innerloop to take */
	if (sample_cursor_delta > 0 && sample_cursor_delta < (1UL << 12))
	{
		u32 ret = mix_bresenham(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);
//		PROFILE_COLOR(31, 0, 0);
		return ret;
	}
	else
	{
		u32 ret = mix_simple(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);
//		PROFILE_COLOR(31, 0, 0);
		return ret;
	}
}

void mixer::clip_samples(s8 *target, s32 *source, u32 samples, u32 dc_offs)
{
	assert(target != NULL);
	assert(source != NULL);
	
	register s32 *src = source;
	register s8  *dst = target;
	register u32 dc_offs_local = dc_offs;
	
	// the compiler is too smart -- we need to prevent it from doing some arm11-optimizations.
	register s32 high_clamp = 127 + dc_offs;
	register s32 low_clamp = -128 + dc_offs;
	
	/* TODO: do this separatly, when _all_ mixing for an entire frame is done. makes profiling a lot easier. */
	/* also consider optimizing this further */
	
#define ITERATION                                 \
	{	                                          \
		s32 samp = (*src++) >> 8;                 \
		if (samp > high_clamp) samp = high_clamp; \
		if (samp < low_clamp) samp = low_clamp;   \
		samp -= dc_offs_local;                    \
		*dst++ = samp;                            \
	}
	PROFILE_COLOR(0, 0, 31);
	
	register u32 s = samples / 16;
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
	PROFILE_COLOR(31, 0, 0);
}
