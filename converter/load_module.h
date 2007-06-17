#ifndef LOAD_MODULE_H
#define LOAD_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h> /* needed for FILE */

struct pimp_module;
struct pimp_sample_bank;

struct pimp_module *load_module_mod(FILE *fp);
struct pimp_module *load_module_xm(FILE *fp, struct pimp_sample_bank *sample_bank);

#ifdef __cplusplus
}
#endif

#endif /* LOAD_MODULE_H */
