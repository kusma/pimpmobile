#ifndef _PIMPMOBILE_H_
#define _PIMPMOBILE_H_

#ifdef __cplusplus
extern "C" {
#endif


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
	unsigned int  length;
	unsigned int  loop_start;
	unsigned int  loop_length;
	unsigned char volume;
	signed char   finetune;
	unsigned char type;
	unsigned char pan;
	signed char   rel_note;
} pimp_channel_t;

void pimp_init();
void pimp_close();

void pimp_vblank(); /* call this on the beginning of each vsync */
void pimp_frame(); /* call once each frame. doesn't need to be called in precious vblank time */

#ifdef __cplusplus
}
#endif

#endif /* _PIMPMOBILE_H_ */
