#ifndef CONFIG_H
#define CONFIG_H

#define CYCLES_PR_FRAME 280896

/* 32 is the maximum amount of channels in fasttracker2. a nice default. */
#define CHANNELS 8

/* check the sample-rate calculator at http://www.pineight.com/gba/samplerates/ for more glitch-free samplerates */
/* 0x4000100 = 0xFFFF, 0x4000102 = 0x0083 */
#define SAMPLERATE (18157.16)

/* only 130 bytes big, quite damn pleasing results */
#define AMIGA_DELTA_LUT_LOG_SIZE 7

/* derivated settings. don't touch. */
#define SOUND_BUFFER_SIZE (CYCLES_PR_FRAME / ((int)((1 << 24) / SAMPLERATE)))
#define AMIGA_DELTA_LUT_SIZE (1 << AMIGA_DELTA_LUT_LOG_SIZE)
#define AMIGA_DELTA_LUT_FRAC_BITS (15 - AMIGA_DELTA_LUT_LOG_SIZE)

/* enable / disable assert */
#ifndef NDEBUG
#define DEBUG_PRINTF_ENABLED
#define ASSERT_ENABLED
#endif

#define DEBUG_PRINTF_ENABLED
// #define ASSERT_ENABLED

#endif /* CONFIG_H */
