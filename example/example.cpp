#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>

#ifndef REG_WAITCNT
#define REG_WAITCNT (*(vu16*)(REG_BASE + 0x0204))
#endif

#include <stdio.h>
#include <assert.h>

#include "../include/pimpmobile.h"
#include "../src/mixer.h"
#include "../src/config.h"
#include "gbfs.h"

extern const u8  sample_bank[];
extern const u8  module[];


void vblank()
{
	pimp_vblank();
	while (REG_VCOUNT != 0);
	pimp_frame();
}

int main()
{
//	REG_WAITCNT = 0x46d6; // lets set some cool waitstates...
	REG_WAITCNT = 0x46da; // lets set some cool waitstates...

	InitInterrupt();
	EnableInterrupt(IE_VBL);
	consoleInit(0, 4, 0, NULL, 0, 15);

	BG_COLORS[0] = RGB5(0, 0, 0);
	BG_COLORS[241] = RGB5(31, 31, 31);
	REG_DISPCNT = MODE_0 | BG0_ON;
	
	GBFS_FILE const* gbfs_file = find_first_gbfs_file((void*)0x08000000);
	
	const void *mod = gbfs_get_obj(gbfs_file, "rhino.mod.bin", 0);
	const void *sb  = gbfs_get_obj(gbfs_file, "sample_bank.bin", 0);
	iprintf("\n\n %i %i\n", mod, sb);
	pimp_init(mod, sb);

	SetInterrupt(IE_VBL, vblank);
	EnableInterrupt(IE_VBL);

	while (1)
	{
		VBlankIntrWait();
	}

	pimp_close();
}
