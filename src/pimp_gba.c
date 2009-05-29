/* pimp_gba.c -- GameBoy Advance c-interface for Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include "pimp_render.h"
#include "pimp_debug.h"
#include <stdio.h>

typedef volatile u16 vu16;
typedef volatile u32 vu32;
#define IWRAM_DATA __attribute__((section(".iwram")))
#define EWRAM_DATA __attribute__((section(".ewram")))

#define DMA_DST_FIXED (2U << 21)
#define DMA_SRC_FIXED (2U << 23)
#define DMA_REPEAT    (1U << 25)
#define DMA32         (1U << 26)
#define DMA_SPECIAL   (3U << 28)
#define DMA_ENABLE    (1U << 31)

#define SNDA_VOL_100    (1U << 2)
#define SNDA_R_ENABLE   (1U << 8)
#define SNDA_L_ENABLE   (1U << 9)
#define SNDA_RESET_FIFO (1U << 11)
#define TIMER_START     (1U << 7)

#define REG_BASE        0x04000000
#define REG_SOUNDCNT_H (*((vu16*)(REG_BASE + 0x082)))
#define REG_SOUNDCNT_X (*((vu16*)(REG_BASE + 0x084)))
#define REG_TM0CNT_L   (*((vu16*)(REG_BASE + 0x100)))
#define REG_TM0CNT_H   (*((vu16*)(REG_BASE + 0x102)))
#define REG_DMA1SAD    (*((vu32*)(REG_BASE + 0x0bc)))
#define REG_DMA1DAD    (*((vu32*)(REG_BASE + 0x0c0)))
#define REG_DMA1CNT    (*((vu32*)(REG_BASE + 0x0c4)))
#define REG_FIFO_A     (*((vu32*)(REG_BASE + 0x0A0)))

void CpuFastSet( const void *src, void *dst, u32 mode)
{
	asm (
		"mov r0, %[src]  \n"
		"mov r1, %[dst]  \n"
		"mov r2, %[mode] \n"
#ifdef __thumb__
		"swi 0xC         \n"
#else
		"swi 0xC << 16   \n"
#endif
		: /* no output */
		: /* inputs */
			[src]  "r"(src),
			[dst]  "r"(dst),
			[mode] "r"(mode)
		: "r0", "r1", "r2", "r3" /* clobbers */
	);
}

static struct pimp_mixer       pimp_gba_mixer IWRAM_DATA;
static struct pimp_mod_context pimp_gba_ctx   EWRAM_DATA;

/* setup some constants */
#define CYCLES_PR_FRAME 280896
#define CYCLES_PR_SAMPLE  ((int)((1 << 24) / ((float)PIMP_GBA_SAMPLERATE)))
#define SOUND_BUFFER_SIZE ((int)((float)CYCLES_PR_FRAME / CYCLES_PR_SAMPLE))

/* mix and playback-buffers */
static s8  pimp_gba_sound_buffers[2][SOUND_BUFFER_SIZE] IWRAM_DATA;
static u32 pimp_gba_sound_buffer_index = 0;
static s32 pimp_gba_mix_buffer[SOUND_BUFFER_SIZE] IWRAM_DATA;

void pimp_gba_init(const struct pimp_module *module, const void *sample_bank)
{
	u32 zero = 0;
	pimp_gba_mixer.mix_buffer = pimp_gba_mix_buffer;
	pimp_mod_context_init(&pimp_gba_ctx, (const pimp_module*)module, (const u8*)sample_bank, &pimp_gba_mixer, PIMP_GBA_SAMPLERATE);
	
	/* call BIOS-function CpuFastSet() to clear buffer */
	CpuFastSet(&zero, &pimp_gba_sound_buffers[0][0], DMA_SRC_FIXED | ((SOUND_BUFFER_SIZE / 4) * 2));
	REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
	REG_SOUNDCNT_X = (1 << 7);
	
	DEBUG_PRINT(DEBUG_LEVEL_INFO, ("samples pr frame: 0x%x\nsound buffer size: %d\n", SAMPLES_PR_FRAME, SOUND_BUFFER_SIZE));
	
	/* setup timer */
	REG_TM0CNT_L = (1 << 16) - CYCLES_PR_SAMPLE;
	REG_TM0CNT_H = TIMER_START;
}

void pimp_gba_close()
{
	REG_SOUNDCNT_X = 0;
}

void pimp_gba_vblank()
{
	if (pimp_gba_sound_buffer_index == 0)
	{
		REG_DMA1CNT = 0;
		REG_DMA1SAD = (u32) &(pimp_gba_sound_buffers[0][0]);
		REG_DMA1DAD = (u32) &REG_FIFO_A;
		REG_DMA1CNT = DMA_DST_FIXED | DMA_REPEAT | DMA32 | DMA_SPECIAL | DMA_ENABLE;
	}
	pimp_gba_sound_buffer_index ^= 1;
}

void pimp_gba_set_callback(pimp_callback in_callback)
{
	pimp_gba_ctx.callback = in_callback;
}

void pimp_gba_set_pos(int row, int order)
{
	pimp_mod_context_set_pos(&pimp_gba_ctx, row, order);
}

int pimp_gba_get_row()
{
	return pimp_mod_context_get_row(&pimp_gba_ctx);
}

int pimp_gba_get_order()
{
	return pimp_mod_context_get_order(&pimp_gba_ctx);
}

void pimp_gba_frame()
{
	static volatile BOOL locked = FALSE;
	if (TRUE == locked) return; /* whops, we're in the middle of filling. sorry. you did something wrong! */
	locked = TRUE;
	
	pimp_render(&pimp_gba_ctx, pimp_gba_sound_buffers[pimp_gba_sound_buffer_index], SOUND_BUFFER_SIZE);
	
	locked = FALSE;
}

#ifndef DEBUG
void pimp_mixer_clear(s32 *target, const u32 samples)
{
	int i;
	static const u32 zero = 0;
	const u32 *src = &zero;
	u32 *dst = (u32*)target;

	/* bit 24 = fixed source address, bit 0-20 = wordcount (must be dividable by 8) */
	const int mode = (1 << 24) | (samples & ~7);
	
	ASSERT(NULL != src);
	ASSERT(NULL != dst);
	
	/* clear the samples that CpuFastSet() can't */
	for (i = samples & 7; i; --i)
	{
		*dst++ = 0;
	}
	if (0 == (samples & ~7)) return;
	
	ASSERT(((int)src & 3) == 0);
	ASSERT(((int)dst & 3) == 0);
	
	/* call BIOS-function CpuFastSet() to clear buffer */
	CpuFastSet(src, dst, mode);
}
#endif
