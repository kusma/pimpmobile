#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "mixer.h"
#include "debug.h"

#include <gba_base.h>
#include <gba_video.h>
int profile_counter = 0;

static u32 mix_simple(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
	assert(target != 0);
	assert(sample_data != 0);
	assert((samples & 7) == 0);
	assert(samples != 0);
	asm(
"\
	b .Ldataskip%=                          \n\
.Lstack_store%=:                            \n\
.align 4                                    \n\
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

	assert(target != 0);
	assert(sample_data != 0);
	assert((samples & 7) == 0);
	assert(samples != 0);
	
	asm(
"\
	b .Ldataskip%=                          \n\
.Lstack_store%=:                            \n\
.align 4                                    \n\
.word                                       \n\
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
		[counter] "r"(samples >> 3),
		[data]    "1"(sample_data),
		[target]  "r"(target),
		[delta]   "r"(sample_cursor_delta << 20),
		[vol]     "r"(vol)
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "sp", "cc"
	);
	return ((sample_data - old_sample_data - 1) << 12) + (sample_cursor >> 20);
}

u32 mixer::mix_samples(s32 *target, u32 samples, const u8 *sample_data, u32 vol, u32 sample_cursor, s32 sample_cursor_delta)
{
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

//	return mix_simple(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);

	/* decide what innerloop to take */
	if (sample_cursor_delta > 0 && sample_cursor_delta < (1 << 12))
	{
		u32 ret = mix_bresenham(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);
		return ret;
	}
	else
	{
		u32 ret = mix_simple(target, samples, sample_data, vol, sample_cursor, sample_cursor_delta);
		return ret;
	}
}
