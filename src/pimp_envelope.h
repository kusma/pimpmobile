#ifndef PIMP_ENVELOPE_H
#define PIMP_ENVELOPE_H

typedef struct
{
	u16 node_tick[25];
	s16 node_magnitude[25];
	s16 node_delta[25];

	u8 node_count;
	u8 flags; // bit 0: loop enable, bit 1: sustain loop enable
	u8 loop_start, loop_end;
	u8 sustain_loop_start, sustain_loop_end;
} pimp_envelope;

#endif /* PIMP_ENVELOPE_H */
