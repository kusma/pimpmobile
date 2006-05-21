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
#include "../src/mixer.h"
#include "../src/config.h"
#include "gbfs.h"

// #define USE_AAS

#ifdef USE_AAS
#include "aas.h"
#include "AAS_Data.h"
#endif

#define CYCLES_PR_FRAME 280896

void profile_start()
{
	REG_TM3CNT_H = 0;
	REG_TM3CNT_L = 0;
	REG_TM3CNT_H = TIMER_START;
}

int profile_stop()
{
	return REG_TM3CNT_L;
}

void timer1()
{
#ifdef USE_AAS
	BG_COLORS[0] = RGB5(31, 0, 0);
	profile_start();
	asm volatile ("MOV R11,R11");
	AAS_Timer1InterruptHandler();
	AAS_DoWork();
	int stop = profile_stop();
	BG_COLORS[0] = RGB5(0, 0, 0);
//	printf("cycles: %i\n", stop);
//	printf("cycles: %f\n", float(stop * 100) / CYCLES_PR_FRAME);
#endif
}



int fade = 0;
void callback(int a, int b)
{
	fade = 255;
}

#define BYTES 256 * 128
char test[(BYTES)] EWRAM_DATA;

void vblank()
{
#ifndef USE_AAS
	pimp_vblank();
#endif

#ifdef USE_AAS
	BG_COLORS[0] = RGB5(31, 0, 0);
//	AAS_DoWork();
	BG_COLORS[0] = RGB5(0, 0, 0);
#else
//	while (REG_VCOUNT != 0);
	BG_COLORS[0] = RGB5(31, 0, 0);
	profile_start();
	pimp_frame();
	int stop = profile_stop();
	BG_COLORS[0] = RGB5(0, 0, 0);
//	printf("cycles: %i\n", stop);
	if (false)
	{
		static float percent = 0.f;
		percent += ((float(stop * 100) / CYCLES_PR_FRAME) - percent) * 0.5f;
		printf("cycles: %2.2f\n", percent);
	}
#endif

	static int frame = 0;
	if (fade > 0) fade -= 8;
	int f = (fade * fade) >> 8;

	BG_COLORS[0] = RGB8(f, f, f);
	
//	memset(test, 0, BYTES);
//	printf("%d\n", pimp_get_row());
	
//	while (REG_VCOUNT != 150);
//	while (REG_VCOUNT != 0);
	
/*
	frame++;
	if (frame == 60)
	{
		printf("%f %% cpu used\n", profile_counter * ((1.f / 60) * (1.f / CYCLES_PR_FRAME) * 100));
		profile_counter = 0;
		frame = 0;
	}
*/
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
//	REG_WAITCNT = 0x46da; // lets set some cool waitstates... (helps only on multi-byte reads (no... ldrb is 5 cycles instead of 6 from rom))
	REG_WAITCNT = (2 << 2) | (0 << 4) | (1 << 14);

	InitInterrupt();
	EnableInterrupt(IE_VBL);
	consoleInit(0, 4, 0, NULL, 0, 15);

	BG_COLORS[0] = RGB5(0, 0, 0);
	BG_COLORS[241] = RGB5(31, 31, 31);
	REG_DISPCNT = MODE_0 | BG0_ON;
	
	fs = find_first_gbfs_file((void*)0x08000000);
	file_count = gbfs_count_objs(fs);
	sample_bank  = gbfs_get_obj(fs, "sample_bank.bin", 0);
	
#ifdef USE_AAS
	AAS_SetConfig(AAS_CONFIG_MIX_16KHZ, AAS_CONFIG_CHANS_4, AAS_CONFIG_SPATIAL_MONO, AAS_CONFIG_DYNAMIC_OFF);
	AAS_MOD_Play(AAS_DATA_MOD_eye);
	SetInterrupt(IE_TIMER1, timer1);
//	EnableInterrupt(IE_TIMER1);
#else
	pimp_set_callback(callback);
	play_next_file();
#endif	

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
