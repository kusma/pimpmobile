#ifndef DEBUG_H
#define DEBUG_H

#include <gba_video.h>

#if 0
 #define PROFILE_COLOR(r, g, b) BG_COLORS[0] = RGB5((r), (g), (b))
#else
 #define PROFILE_COLOR(r, g, b)
#endif

#ifdef DEBUG_PRINTF_ENABLED
 #define DEBUG_PRINT(x) iprintf x
#else
 #define DEBUG_PRINT(x)
#endif

#ifndef ASSERT_ENABLED
 #define ASSERT(expr, X) \
	do{                  \
		if (!(expr)) DEBUG_PRINT("*** ASSERT FAILED %s AT (%s:%i)\n", #expr, __FILE__, __LINE__); \
	} while(0)
#else
 #define ASSERT(expr, X)
#endif


#endif /* DEBUG_H */
