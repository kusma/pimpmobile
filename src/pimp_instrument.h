/* pimp_instrument.h -- Instrument data structure and getter functions
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef PIMP_INSTRUMENT_H
#define PIMP_INSTRUMENT_H

#include "pimp_types.h"

struct pimp_instrument
{
	pimp_rel_ptr sample_ptr;
	pimp_rel_ptr vol_env_ptr;
	pimp_rel_ptr pan_env_ptr;

#if 0 /* IT ONLY (later) */
	pimp_rel_ptr pitch_env_ptr;
#endif

	u16 volume_fadeout;
	
	u8 sample_count;                 /* number of samples tied to instrument */
	u8 sample_map[120];

#if 0 /* IT ONLY (later) */
	new_note_action_t        new_note_action;
	duplicate_check_type_t   duplicate_check_type;
	duplicate_check_action_t duplicate_check_action;
	
	s8 pitch_pan_separation; /* no idea what this one does */
	u8 pitch_pan_center;     /* not this on either; this one seems to be a note index */
#endif

};

#include "pimp_internal.h"
#include "pimp_debug.h"
#include "pimp_envelope.h"
#include "pimp_sample.h"

static INLINE struct pimp_sample *pimp_instrument_get_sample(const struct pimp_instrument *instr, int i)
{
	ASSERT(instr != NULL);
	return &((struct pimp_sample*)PIMP_GET_PTR(instr->sample_ptr))[i];
}

static INLINE struct pimp_envelope *pimp_instrument_get_vol_env(const struct pimp_instrument *instr)
{
	ASSERT(instr != NULL);
	return (struct pimp_envelope*)(instr->vol_env_ptr == 0 ? NULL : PIMP_GET_PTR(instr->vol_env_ptr));
}

static INLINE struct pimp_envelope *pimp_instrument_get_pan_env(const struct pimp_instrument *instr)
{
	ASSERT(instr != NULL);
	return (struct pimp_envelope*)(instr->pan_env_ptr == 0 ? NULL : PIMP_GET_PTR(instr->pan_env_ptr));
}

#endif /* PIMP_INSTRUMENT_H */
