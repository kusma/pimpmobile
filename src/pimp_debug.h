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

#include "pimp_module.h"

void print_pattern_entry(const pimp_pattern_entry *pe);
void print_pattern(const pimp_module *mod, pimp_pattern *pat);

#endif /* PIMP_DEBUG_H */
