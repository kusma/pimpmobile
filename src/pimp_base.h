/* pimp_base.h -- Some base defines used in Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_BASE_H
#define PIMP_BASE_H

#include "pimp_types.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE ((BOOL)1)
#endif

#ifndef FALSE
#define FALSE ((BOOL)0)
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef INLINE
#define INLINE __inline
#endif

#ifndef UNIT_TESTING
#define STATIC static
#else
#define STATIC
#endif

#ifndef PURE
#define PURE __attribute__((pure))
#endif

#endif /* PIMP_BASE_H */
