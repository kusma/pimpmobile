#ifndef DEBUG_H
#define DEBUG_H

#include <gba_video.h>

#if 0
#define PROFILE_COLOR(r, g, b) BG_COLORS[0] = RGB5((r), (g), (b))
#else
#define PROFILE_COLOR(r, g, b)
#endif

#ifdef ENABLE_DEBUG_PRINTING
#define DEBUG_PRINT(x) iprintf x
#else
#define DEBUG_PRINT(x)
#endif

#endif /* DEBUG_H */
