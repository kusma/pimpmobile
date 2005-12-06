#ifndef _PIMPMOBILE_H_
#define _PIMPMOBILE_H_

#ifdef __cplusplus
extern "C"
{
#endif

void pimp_init(void *module, void *sample_bank);
void pimp_close();

void pimp_vblank(); /* call this on the beginning of each vsync */
void pimp_frame(); /* call once each frame. doesn't need to be called in precious vblank time */

#ifdef __cplusplus
}
#endif

#endif /* _PIMPMOBILE_H_ */
