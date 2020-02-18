/* pimp_config.h -- Compile-time configuration of Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_CONFIG_H
#define PIMP_CONFIG_H

/* 32 is the maximum amount of channels in fasttracker2. a nice default. */
#define PIMP_CHANNEL_COUNT 32

/* check the sample-rate calculator at http://www.pineight.com/gba/samplerates/ for more glitch-free periods */

#if 0
#define PIMP_GBA_PERIOD (532)  /* 31536.12 */
#define PIMP_GBA_PERIOD (627)  /* 26757.92 */
#define PIMP_GBA_PERIOD (798)  /* 21024.08 */
#define PIMP_GBA_PERIOD (836)  /* 20068.44 */
#endif
#define PIMP_GBA_PERIOD (924)  /* 18157.16 */
#if 0
#define PIMP_GBA_PERIOD (1254) /* 13378.96 */
#define PIMP_GBA_PERIOD (1463) /* 11467.68 */
#define PIMP_GBA_PERIOD (1596) /* 10512.04 */
#endif

/* enable / disable assert */
/* #define DEBUG_PRINT_ENABLE */
/* #define ASSERT_ENABLE */
/* #define PRINT_PATTERNS */

#define PIMP_MIXER_IRQ_SAFE /* on by default */
#define PIMP_MIXER_USE_BRESENHAM_MIXER
/* #define PIMP_MIXER_NO_MIXING */
/* #define PIMP_MIXER_NO_CLIPPING */

#endif /* PIMP_CONFIG_H */
