#ifndef PIMP_RENDER
#define PIMP_RENDER

#include "pimp_internal.h"
#include "pimp_mod_context.h"

#ifdef __cplusplus
extern "C" {
#endif

void __pimp_render(pimp_mod_context *ctx, s8 *buf, u32 samples);

#ifdef __cplusplus
}
#endif

#endif /* PIMP_RENDER */
