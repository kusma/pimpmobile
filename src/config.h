#ifndef _PIMP_CONFIG_H_
#define _PIMP_CONFIG_H_

#define CYCLES_PR_FRAME 280896

/* 32 is the maximum amount of channels in fasttracker2. a nice default. */
#define CHANNELS   32

/* check the sample-rate calculator at http://www.pineight.com/gba/samplerates/ for more glitch-free samplerates */
#define SAMPLERATE 18157

/* only 130 bytes big, quite damn pleasing results */
#define AMIGA_FREQ_TABLE_LOG_SIZE 7

/* derivated settings. don't touch. */
#define SOUND_BUFFER_SIZE (CYCLES_PR_FRAME / ((1 << 24) / SAMPLERATE))
#define AMIGA_FREQ_TABLE_SIZE (1 << AMIGA_FREQ_TABLE_LOG_SIZE)
#define AMIGA_FREQ_TABLE_FRAC_BITS (15 - AMIGA_FREQ_TABLE_LOG_SIZE)

#endif /* _PIMP_CONFIG_H_ */
