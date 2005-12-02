#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>

#include <stdio.h>
#include <assert.h>

#include "include/pimpmobile.h"
#include "src/mixer.h"

extern const u8  sample[];
extern const u8  sample_end[];

void vblank()
{
	BG_COLORS[0] = RGB5(0, 0, 31);
	pimp_vblank();
	BG_COLORS[0] = RGB5(0, 0, 0);

	while (REG_VCOUNT != 0);
//	while (REG_VCOUNT != 100);

	BG_COLORS[0] = RGB5(0, 31, 0);
	pimp_frame();
	BG_COLORS[0] = RGB5(0, 0, 0);
}

const u8 samp_data[2] IWRAM_DATA = {0, 255};

#define REG_WAITCNT *(vu16*)(REG_BASE + 0x0204)

int main()
{
//	REG_WAITCNT = 0x46d6; // lets set some cool waitstates...
	REG_WAITCNT = 0x46da; // lets set some cool waitstates...

	InitInterrupt();
	EnableInterrupt(IE_VBL);
	consoleInit(0, 4, 0, NULL, 0, 15);

	BG_COLORS[0]=RGB5(0, 0, 0);
	BG_COLORS[241]=RGB5(31, 31, 31);
	REG_DISPCNT = MODE_0 | BG0_ON;

	pimp_init();

	mixer::sample_t mixer_sample;
	mixer_sample.data = sample;
	mixer_sample.len = &sample_end[0] - &sample[0];

/*	mixer_sample.data = samp_data;
	mixer_sample.len = 2; */

	mixer_sample.loop_type = mixer::LOOP_TYPE_FORWARD;
	mixer_sample.loop_start = 0;
	mixer_sample.loop_end = (&sample_end[0] - &sample[0]);

	mixer::channels[0].sample_cursor = 0;
	mixer::channels[0].sample_cursor_delta = 1 << 12;
	mixer::channels[0].volume = 255;
	mixer::channels[0].sample = &mixer_sample;

/*
	mixer::channels[1].sample_cursor = 0;
	mixer::channels[1].sample_cursor_delta = 5 << 10;
	mixer::channels[1].volume = 64;
	mixer::channels[1].sample = &mixer_sample;
*/
	SetInterrupt(IE_VBL, vblank);
	EnableInterrupt(IE_VBL);

	while (1)
	{
		VBlankIntrWait();
	}

	pimp_close();
}
