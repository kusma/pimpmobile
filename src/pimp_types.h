#ifndef PIMP_TYPES_H
#define PIMP_TYPES_H

typedef   signed char      s8;
typedef unsigned char      u8;
typedef   signed short     s16;
typedef unsigned short     u16;
typedef   signed int       s32;
typedef unsigned int       u32;
typedef   signed long long s64;
typedef unsigned long long u64;

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef INILIE
#define INLINE inline
#endif

#endif /* PIMP_TYPES_H */
