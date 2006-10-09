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

#include "../include/pimpmobile.h"
#include "../src/pimp_mixer.h"
#include "../src/pimp_config.h"
#include "gbfs.h"

int fade = 0;
void callback(int a, int b)
{
	fade = 255;
}

const void *mod;
GBFS_FILE const* fs;
const void *sample_bank = 0;
int curr_file = 0;
int file_count = 0;

void vblank()
{
	pimp_vblank();
	if (fade > 0) fade -= 8;
	int f = (fade * fade) >> 8;

	while (REG_VCOUNT != 0);

	REG_TM2CNT_L = 0;
	REG_TM2CNT_H = 0;
	REG_TM2CNT_H = TIMER_START;
//	BG_COLORS[0] = RGB8(31, 31, 0);
	pimp_frame();
//	BG_COLORS[0] = RGB8(0, 0, 0);
	u32 value = REG_TM2CNT_L;
	BG_COLORS[0] = RGB8(f, f, f);
	
	static int counter = 0;
	
	static int accum = 0;
//	float val = float(value) / ((1 << 24) / 60);
	accum += value;

	counter++;
	if (counter == 60)
	{

		printf("%d\n", accum);
//		printf("%d.%02d\n", int(val * 100), int(val * 100 * 100));

//		pimp_set_pos(0, 0);
	//	pimp_close();
//		pimp_init(mod, sample_bank);

		counter = 0;
		accum = 0;
	}
	
//	printf("%d\n", value);
}

void play_next_file()
{
	static char name[32];

	do
	{
		mod = gbfs_get_nth_obj(fs, curr_file++, name, 0);
		if (curr_file > file_count - 1) curr_file = 0;
	}
	while (strncmp(name, "sample_bank.bin", 32) == 0);
	
	pimp_close();
	pimp_init(mod, sample_bank);
	pimp_set_callback(callback);
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
		int keys = KeysDown();
		if (keys & KEY_RIGHT) pimp_set_pos(0, pimp_get_order() + 1);
		if (keys & KEY_LEFT)  pimp_set_pos(pimp_get_row() + 8, pimp_get_order());
		if (keys & KEY_A) play_next_file();
	}

	pimp_close();
}
