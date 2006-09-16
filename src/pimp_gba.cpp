#include "pimp_render.h"

#include <gba_dma.h>
#include <gba_sound.h>
#include <gba_timers.h>
#include <gba_systemcalls.h>

#include <stdio.h>

pimp_mod_context __pimp_ctx;
pimp_mixer       __pimp_mixer;

STATIC s8  __pimp_sound_buffers[2][SOUND_BUFFER_SIZE] IWRAM_DATA;
STATIC u32 __pimp_sound_buffer_index = 0;

extern "C" void pimp_init(const void *module, const void *sample_bank)
{
	__pimp_mod_context_init(&__pimp_ctx, (const pimp_module*)module, (const u8*)sample_bank, &__pimp_mixer);

	u32 zero = 0;
	CpuFastSet(&zero, &__pimp_sound_buffers[0][0], DMA_SRC_FIXED | ((SOUND_BUFFER_SIZE / 4) * 2));
	REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
	REG_SOUNDCNT_X = (1 << 7);
	
	/* setup timer-shit */
	REG_TM0CNT_L = (1 << 16) - int((1 << 24) / SAMPLERATE);
	REG_TM0CNT_H = TIMER_START;
}

extern "C" void pimp_close()
{
	REG_SOUNDCNT_X = 0;
}

extern "C" void pimp_vblank()
{
	if (__pimp_sound_buffer_index == 0)
	{
		REG_DMA1CNT = 0;
		REG_DMA1SAD = (u32) &(__pimp_sound_buffers[0][0]);
		REG_DMA1DAD = (u32) &REG_FIFO_A;
		REG_DMA1CNT = DMA_DST_FIXED | DMA_REPEAT | DMA32 | DMA_SPECIAL | DMA_ENABLE;
	}
	__pimp_sound_buffer_index ^= 1;
}

extern "C" void pimp_set_callback(pimp_callback in_callback)
{
	__pimp_ctx.callback = in_callback;
}

extern "C" void pimp_set_pos(int row, int order)
{
	__pimp_mod_context_set_pos(&__pimp_ctx, row, order);
}

extern "C" int pimp_get_row()
{
	return __pimp_mod_context_get_row(&__pimp_ctx);
}

extern "C" int pimp_get_order()
{
	return __pimp_mod_context_get_order(&__pimp_ctx);
}

extern "C" void pimp_frame()
{
	static volatile BOOL locked = FALSE;
	if (TRUE == locked) return; /* whops, we're in the middle of filling. sorry. you did something wrong! */
	locked = TRUE;

	__pimp_render(&__pimp_ctx, __pimp_sound_buffers[__pimp_sound_buffer_index], SOUND_BUFFER_SIZE);
	
	locked = FALSE;
}
