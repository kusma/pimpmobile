#ifndef PIMP_DEBUG_H
#define PIMP_DEBUG_H

#ifdef DEBUG_PRINTF_ENABLED
 #define DEBUG_PRINT(X) iprintf X
#else
 #define DEBUG_PRINT(X)
#endif

#ifdef ASSERT_ENABLED
 #include <stdlib.h>
 #define ASSERT(expr) \
	do { \
		if (!(expr)) iprintf("*** ASSERT FAILED %s AT (%s:%i)\n", #expr, __FILE__, __LINE__); \
	} while(0)
#else
 #define ASSERT(expr)
#endif

#endif /* PIMP_DEBUG_H */
