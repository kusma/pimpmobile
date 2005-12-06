#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include "config.h"

typedef struct
{
	unsigned int  length;
	unsigned int  loop_start;
	unsigned int  loop_length;
	unsigned char volume;
	signed char   finetune;
	unsigned char type;
	unsigned char pan;
	signed char   rel_note;
} pimp_sample_t;

typedef struct
{
	
} pimp_pattern_entry_t;

typedef struct
{
	
} pimp_module_t;

typedef struct
{
	u8 note;
	u8 effect;
	u8 effect_param;
	s32 period;        // signed so we can check for underfow.
	s32 porta_speed;   // signed so we can check for underfow.
} pimp_channel_t;

typedef enum
{
	EFF_NONE       = 0x00,
	EFF_PORTA_UP   = 0x01,
	EFF_PORTA_DOWN = 0x02,
	EFF_PORTA_NOTE = 0x03,
	EFF_VIBRATO    = 0x04,
} pimp_effect_t;

/*
    (0..F for the effect byte are the usual hex values,
      the values continue naturally upwards with G,H,I...)
    0xx: no effect at all (warning: MOD/XM uses effect byte 0 for
         arpeggio; they need to be remapped to effect 24)
    1xx: porta up
    2xx: porta down
    3xx: tone porta
    4xy: vibrato: x=rate, y=depth
    5xx: tone porta + volume-slide (command byte is for slide, the tone porta
           is remembered)
    6xx: vibrato + volume-slide (comamnd byte is for slide, the vibrato
           is remembered)
    7xy: tremolo: x=rate, y=depth
    8xx: set panning position (8xx is strictly speaking ignored in Protracker
          MOD files, but we are going to obey it anyway)
    9xx: set sample offset (obeyed only together with new note)
    Axx: volume slide command. The command byte follows the format
          of the ST3 volume command Dxx, implying that for MOD/XM,
          we must restore the volume byte to a valid value
    Bxx: jump to order (for MOD/S3M/XM but not IT, we must convert
           this value from BCD to binary when loading pattern)
    Cxx: set volume
    Dxx: pattern break
    Exy: MultiFX: x = effect selector: values of x:
          0x: Amiga filter on/off (ignored; according to everybody, it sucks)
          1x: fine porta up
          2x: fine porta down
          3x: glissando control (0=off, 1=on)
          4x: set vibrato waveform/control:
               0=sine, 1=ramp-down, 2=square, 3=random
               +4: vibrato waveform position is not reset on new-note
          5x: set fine-tune for note
          6x: pattern loop command (x=0 marks start of loop,
               x != 0 marks end of loop, with x=loop iterations to apply)
          7x: set tremolo control (same syntax as vibrato control)
          8x: normally unused; Digitrakker uses this command
               to control the looping mode of the currently-playing sample
               (0=off, 1=forward, 3=pingpong)
          9x: retrig note
          Ax: fine volume slide up
          Bx: fine volume slide down
          Cx: note cut after x ticks
          Dx: note delay by x ticks
          Ex: pattern delay by x frames
          Fx: FunkInvert (does anyone actually have an idea what this does?)

    Fxx: set tempo/speed (00-1F: set tempo, 20-FF: set BPM)
    Gxx: set global volume (00..40)
    Hxy: global volume slide command
    Ixx:
    Jxx:
    Kxx: note key-off after xx ticks (not really documented)
    Lxx: set envelope position (# of ticks)
    Pxy: panning slide (x=left slide, y=right slide)
    Rxy: multi retrig note:
          x=volume modifier per retrig
             0=none       8="unused"
             1=-1         9=+1
             2=-2         A=+2
             3=-4         B=+4
             4=-8         C=+8
             5=-16        D=+16
             6=*2/3       E=*3/2
             7=*1/2       F=*2
          y=ticks per retrig
    Txy: Tremor x=on-time, y=off-time (+1 for S3M, +2 for XM)
    Xxy: Multi command again, for values of X:
          1y: extra fine portamento up
          2y: extra fine portamento down

   -- following effect bytes are given in hex.
   24xx: arpeggio (0=remembers previous value)
   25xx: set tempo (not limited to range 0..1F unlike effect Fxx)
   26xx: set BPM (not limited to range 20..FF unlike effect Fxx)
   27xx: set channel volume (impulse-tracker)
   28xx: slide channel volume (impulse-tracker)
   29xx: panning vibrato (impulse-tracker)
   2Axx: fine vibrato
   2Bxy: multi-effect (impulse-tracker only)
          5x: panning vibrato waveform( sane as vibrato/tremolo waveform)
          70: past note cut
          71: past note off
          72: past note fade
          73: set NNA to note cut
          74: set NNA to continue
          75: set NNA to note-off
          76: set NNA to note-fade
          77: turn off volume envelope
          78: turn on volume envelope
          79: turn off panning envelope
          7A: turn on panning envelope
          7B: turn off pitch envelope
          7C: turn on pitch envelope
          91: "set surround sound" ???
*/

#endif /* _INTERNAL_H_ */
