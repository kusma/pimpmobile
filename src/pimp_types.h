/* pimp_types.h -- Base types for use internally in Pimpmobile
 * Copyright (C) 2006 J�rn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_TYPES_H
#define PIMP_TYPES_H

#include "pimp_debug.h"
#include <stddef.h>

#ifndef PIMP_DONT_DECLARE_BASIC_TYPES
typedef   signed char      s8;
typedef unsigned char      u8;
typedef   signed short     s16;
typedef unsigned short     u16;
typedef   signed int       s32;
typedef unsigned int       u32;
#endif
typedef   signed long long s64;
typedef unsigned long long u64;

typedef size_t    pimp_size_t;
typedef ptrdiff_t pimp_rel_ptr;

#endif /* PIMP_TYPES_H */
