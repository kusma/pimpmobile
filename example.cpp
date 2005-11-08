#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>

// #include <mappy.h>
#include <stdio.h>
#include <assert.h>
#include "include/pimpmobile.h"

#include "src/mixer.h"

extern const s8  sample[];
extern const s8  sample_end[];
extern const u16 sample_size;

void vblank()
{
	BG_COLORS[0] = RGB5(0, 0, 31);
	pimp_vblank();
	BG_COLORS[0] = RGB5(0, 0, 0);
	
	while (REG_VCOUNT != 0);
	
	BG_COLORS[0] = RGB5(0, 31, 0);
	pimp_frame();
	BG_COLORS[0] = RGB5(0, 0, 0);
}

const u8 samp_data[2] IWRAM_DATA = {0, 255};
mixer::sample_t da_sample;

#define REG_WAITCNT *(vs16*)(REG_BASE + 0x0204)

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
	SetInterrupt(IE_VBL, vblank);
	EnableInterrupt(IE_VBL);

	da_sample.data = sample;
	da_sample.len = &sample_end[0] - &sample[0];
	da_sample.loop_type = mixer::LOOP_TYPE_PINGPONG;
	da_sample.loop_start = (&sample_end[0] - &sample[0]) / 2;
	da_sample.loop_end = (&sample_end[0] - &sample[0]);
	
	iprintf("ballesatan %u\n", &sample_end[0] - &sample[0]);

#if 0
	mixer::channels[0].sample_cursor = 0;
//	mixer::channels[0].sample_cursor_delta = 3 << 11;
	mixer::channels[0].sample_cursor_delta = 7 << 9;
	mixer::channels[0].volume = 127;
	mixer::channels[0].sample = &da_sample;
#endif

	for (u32 i = 0; i < 1; ++i)
	{
		mixer::channels[i].sample_cursor = 0;
		mixer::channels[i].sample_cursor_delta = 5 << 9;
//		mixer::channels[i].sample_cursor_delta = (6 + i) << 8;
		mixer::channels[i].volume = 127;
		mixer::channels[i].sample = &da_sample;
	}
	
	while (1)
	{
		VBlankIntrWait();
	}
	pimp_close();
}
