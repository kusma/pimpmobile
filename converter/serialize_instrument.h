/* serialize_instrument.h -- Serializer for instrument data
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

#ifndef DUMP_INSTRUMENT_H
#define DUMP_INSTRUMENT_H

struct serializer;

/* serialize a sample */
int serialize_sample(struct serializer *s, const struct pimp_sample *samp);

/* serialize all samples in an instrument */
void serialize_instrument_data(struct serializer *s, const struct pimp_instrument *instr);

/* serialize instrument structure */
void serialize_instrument(struct serializer *s, const struct pimp_instrument *instr);

#endif /* DUMP_INSTRUMENT_H */
