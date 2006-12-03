#ifndef DUMP_H
#define DUMP_H

#include <stdlib.h>

void dump_init();
void check_size(int needed_size);
void align(int alignment);
int  get_pos();
const void *get_data();

void dump_byte(unsigned char b);
void dump_halfword(unsigned short h);
void dump_word(unsigned int w);
void dump_string(const char *str, const size_t len);
void dump_pointer(void *ptr);
void set_pointer(void *ptr, int pos);

void fixup_pointers();

#endif /* DUMP_H */
