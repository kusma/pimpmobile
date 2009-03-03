/* example.cpp -- Pimpmobile GameBoy Advance example
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_timers.h>

#ifndef REG_WAITCNT
#define REG_WAITCNT (*(vu16*)(REG_BASE + 0x0204))
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../include/pimp_gba.h"
#include "../converter/load_module.h"
#include "../src/pimp_sample_bank.h"
#include "gbfs_stdio.h"

int accum = 0;
void mix()
{
	REG_TM2CNT_L = 0;
	REG_TM2CNT_H = 0;
	REG_TM2CNT_H = TIMER_START;
	pimp_gba_vblank();
	pimp_gba_frame();
	u32 value = REG_TM2CNT_L;
	accum += value;
}

void report()
{
	float percent = float(accum * 100) / (1 << 24);
	iprintf("cpu: %d.%03d%% (%d c/f)\n", int(percent), int(percent * 1000) % 1000, accum / 60);
	accum = 0;
}

void timer3()
{
	report();
}

void vblank()
{
	mix();
}

void callback(int type, int data)
{
	switch (type)
	{
		case PIMP_CALLBACK_SYNC:
		break;
		
		case PIMP_CALLBACK_UNSUPPORTED_EFFECT:
			iprintf("eff: %X\n", data);
		break;
		
		case PIMP_CALLBACK_UNSUPPORTED_VOLUME_EFFECT:
			iprintf("vol eff: %X\n", data);
		break;
	}
}

int main()
{
//	REG_WAITCNT = 0x46d6; // lets set some cool waitstates...
	REG_WAITCNT = 0x46da; // lets set some cool waitstates...
	
	irqInit();
	irqEnable(IRQ_VBLANK);
	consoleInit(0, 4, 0, NULL, 0, 15);

	BG_COLORS[0] = RGB5(0, 0, 0);
	BG_COLORS[241] = RGB5(31, 31, 31);
	REG_DISPCNT = MODE_0 | BG0_ON;

	gbfs_init(1);
	
	pimp_sample_bank sb;
	pimp_sample_bank_init(&sb);
	
//	FILE *fp = fopen("dxn-oopk.xm", "rb");
	FILE *fp = fopen("biomech.mod", "rb");
	if (!fp)
	{
		fprintf(stderr, "file not found\n");
		return 1;
	}
	pimp_module *mod = load_module_mod(fp, &sb);
	fclose(fp);
	fp = NULL;
	
	if (NULL == mod)
	{
		fprintf(stderr, "failed to load module\n");
		return 1;
	}

	pimp_gba_init(mod, sb.data);
	pimp_gba_set_callback(callback);

	irqSet(IRQ_TIMER3, timer3);
	irqEnable(IRQ_TIMER3);
	REG_TM3CNT_L = 0;
	REG_TM3CNT_H = TIMER_START | TIMER_IRQ | 2;
	
	irqSet(IRQ_VBLANK, vblank);
	irqEnable(IRQ_VBLANK);
	
	while (1)
	{
		VBlankIntrWait();
		scanKeys();
		int keys = keysDown();
		if (keys & KEY_RIGHT) pimp_gba_set_pos(0, pimp_gba_get_order() + 1);
		if (keys & KEY_LEFT)  pimp_gba_set_pos(pimp_gba_get_row() + 8, pimp_gba_get_order());
	}
	
	pimp_gba_close();
	return 0;
}
