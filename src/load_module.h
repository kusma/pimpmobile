/* load_module.h -- Module loader header file
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef LOAD_MODULE_H
#define LOAD_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h> /* needed for FILE */

struct pimp_module;
struct pimp_sample_bank;

struct pimp_module *load_module_mod(FILE *fp, struct pimp_sample_bank *sample_bank);
struct pimp_module *load_module_xm(FILE *fp, struct pimp_sample_bank *sample_bank);

#ifdef __cplusplus
}
#endif

#endif /* LOAD_MODULE_H */
