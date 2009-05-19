/* serializer.h -- GameBoy Advance c-interface for Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef SERIALIZER_H
#define SERIALIZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

struct serializer
{
	unsigned buffer_size;
	unsigned char *data;
	unsigned pos;
};

void serializer_init(struct serializer *s);
void serializer_deinit(struct serializer *s);

void serializer_check_size(struct serializer *s, size_t needed_size);
void serializer_align(struct serializer *s, int alignment);
void serializer_set_pointer(struct serializer *s, void *ptr, int pos);
void serializer_fixup_pointers(struct serializer *s);

void serialize_byte(struct serializer *s, unsigned char b);
void serialize_halfword(struct serializer *s, unsigned short h);
void serialize_word(struct serializer *s, unsigned int w);
void serialize_string(struct serializer *s, const char *str, const size_t len);
void serialize_pointer(struct serializer *s, void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* SERIALIZER_H */
