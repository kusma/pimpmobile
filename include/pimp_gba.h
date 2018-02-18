/*

pimp_gba.h -- The interface of Pimpmobile, a module playback library
targeting the Nintendo GameBoy Advance


Copyright (c) 2005 Jørn Nystad and Erik Faye-Lund

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use
of this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim
   that you wrote the original software. If you use this software in a product,
   an acknowledgment in the product documentation would be appreciated but is
   not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

*/

#ifndef PIMP_GBA_H
#define PIMP_GBA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "pimp_types.h"

void pimp_gba_init(const struct pimp_module *module, const void *sample_bank);
void pimp_gba_close(void);

void pimp_gba_vblank(void); /* call this on the beginning of each vsync */
void pimp_gba_frame(void); /* call once each frame. doesn't need to be called in precious vblank time */

/* get information about playback */
int pimp_gba_get_row(void);
int pimp_gba_get_order(void);
void pimp_gba_set_pos(int row, int order);

/* callback system (for music sync) */
void pimp_gba_set_callback(pimp_callback callback);

#ifdef __cplusplus
}
#endif

#endif /* PIMP_GBA_H */
