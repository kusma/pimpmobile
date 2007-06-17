#ifndef DUMP_INSTRUMENT_H
#define DUMP_INSTRUMENT_H

struct serializer;

/* serialize a sample */
int serialize_sample(struct serializer *s, const pimp_sample *samp);

/* serialize all samples in an instrument */
void serialize_instrument_data(struct serializer *s, const pimp_instrument *instr);

/* serialize instrument structure */
void serialize_instrument(struct serializer *s, const pimp_instrument *instr);

#endif /* DUMP_INSTRUMENT_H */
