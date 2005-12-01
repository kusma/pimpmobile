#include <stdio.h>
#include <stdlib.h>
#include <math.h>


typedef unsigned int u32;

void mix(char *src, u32 pos, u32 delta_frac, char *dst, int samples)
	{
	int smp = *src;
	u32 delta = 0;
	do	{
		delta += delta_frac;
		if( delta < delta_frac )
			smp = *src++;
		*dst = smp;
		}
	while(--samples);
	}


u32 slow_modulo( u32 A, u32 B )
	{
	return A % B;
	}



u32 mini_modulo( u32 A, u32 B )
	{
	u32 res;
	switch( B )
		{
		case 2: res = A % 2; break;
		case 3: res = A % 3; break;
		case 4: res = A % 4; break;
		case 5: res = A % 5; break;
		case 6: res = A % 6; break;
		case 7: res = A % 7; break;
		case 8: res = A % 8; break;
		case 9: res = A % 9; break;
		case 10: res = A % 10; break;
		case 11: res = A % 11; break;
		case 12: res = A % 12; break;
		case 13: res = A % 13; break;
		case 14: res = A % 14; break;
		case 15: res = A % 15; break;
		default: res = 0; break;
		}
	return res;
	}



// translate a MOD-file period to a note-index

// 856 -> return C-4 (numerical value 49 )
// 428 -> return C-5 (numerical value 61 )

// 428 corresponds to
//       "C-2" in protracker,
//       "C-4" in fasttracker and scream-tracker,
//       "C-5" in impulse-tracker
// so we call it C-5.

// for period indices that can actually appear in a MOD-file, this function
// can return notes in the range A-1 to B-9; any notes not in the
// range C-3 to B-5 indicate that the module is of non-Protracker origin
// and thus should not be subject to Amiga frequency clamps.

int return_nearest_note(int p)
	{
	if( p < 14 ) return -1; // this is not a note
	double log2p = log(p)/log(2);
	double note = (log2( 428 << 5) - log2p) * 12.0 + 1;
	int note_int = floor( note + .5 );
	if( note_int <= 0 || note_int > 120 )
		return -1; // this is not a note
	return note_int;
	}


void print_note( int p0 )
	{
	int p = return_nearest_note( p0 );
	int note_within_octave = (p-1)%12;
	int octave = (p-1) / 12;
	static const char *note_names[12] = {
		"C-", "C#", "D-", "D#",
		"E-", "F-", "F#", "G-",
		"G#", "A-", "A#", "B-" };
	printf("period=%d, note=%s%d\n", p0, note_names[note_within_octave], octave );
	}



int main(void)
	{
	int i;
	float P = 856.3;
	for(i=0;i<36;i++)
		{
		int v = floor( P );
		if(!(i % 12))
			printf("\n");
		printf( "%d, ", v);
		P *= pow( 2.0, -1.0/12.0 );
		}
	printf("\n");
	for(i=14;i<28;i++)
		print_note( i );
	print_note(4095);
	}










