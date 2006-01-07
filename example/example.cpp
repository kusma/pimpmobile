#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>

#ifndef REG_WAITCNT
#define REG_WAITCNT (*(vu16*)(REG_BASE + 0x0204))
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../include/pimpmobile.h"
#include "../src/mixer.h"
#include "../src/config.h"
#include "gbfs.h"

void vblank()
{
	pimp_vblank();
	while (REG_VCOUNT != 0);
	pimp_frame();
}

GBFS_FILE const* fs;
const void *sample_bank = 0;
int curr_file = 0;
int file_count = 0;

void play_next_file()
{
	static char name[32];
	const void *mod;

	do
	{
		mod = gbfs_get_nth_obj(fs, curr_file++, name, 0);
		if (curr_file > file_count - 1) curr_file = 0;
	}
	while (strncmp(name, "sample_bank.bin", 32) == 0);
	
	pimp_close();
	pimp_init(mod, sample_bank);
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
	
	fs = find_first_gbfs_file((void*)0x08000000);
	file_count = gbfs_count_objs(fs);
	sample_bank  = gbfs_get_obj(fs, "sample_bank.bin", 0);
	
	play_next_file();

	SetInterrupt(IE_VBL, vblank);
	EnableInterrupt(IE_VBL);
	
	while (1)
	{
		VBlankIntrWait();
		ScanKeys();
		if (KeysDown() & KEY_A) play_next_file();
	}

	pimp_close();
}
