/* pimp_render.h -- Interface for the actual audio-rendering in Pimpmobile
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_RENDER
#define PIMP_RENDER

#include "pimp_internal.h"
#include "pimp_mod_context.h"

#ifdef __cplusplus
extern "C" {
#endif

void pimp_render(struct pimp_mod_context *ctx, s8 *buf, u32 samples);

#ifdef __cplusplus
}
#endif

#endif /* PIMP_RENDER */
