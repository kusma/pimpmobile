/* pimp_config.h -- Compile-time configuration of Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_CONFIG_H
#define PIMP_CONFIG_H

/* 32 is the maximum amount of channels in fasttracker2. a nice default. */
#define CHANNELS 32

/* check the sample-rate calculator at http://www.pineight.com/gba/samplerates/ for more glitch-free samplerates */
/* 0x4000100 = 0xFFFF, 0x4000102 = 0x0083 */
/* #define SAMPLERATE (18157.16) */
/* #define SAMPLERATE (16384) */
#define SAMPLERATE 15768.06

/* only 130 bytes big, quite damn pleasing results */
#define AMIGA_DELTA_LUT_LOG_SIZE 7

/* derivated settings. don't touch. */
#define AMIGA_DELTA_LUT_SIZE (1 << AMIGA_DELTA_LUT_LOG_SIZE)
#define AMIGA_DELTA_LUT_FRAC_BITS (15 - AMIGA_DELTA_LUT_LOG_SIZE)

/* enable / disable assert */
/*
#define DEBUG_PRINT_ENABLE
#define ASSERT_ENABLE
*/

#endif /* PIMP_CONFIG_H */
