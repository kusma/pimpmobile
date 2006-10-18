/* pimp_mixer_clip_arm.S -- ARM assembly optimized sample clipping code
 * Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

.text
.section .iwram
.arm
.align 2

@ samples to align = 3 - (DST & 3)

.clipper_align_jumptable: @ used to align the pointer
.word .clip0
.word .clip3
.word .clip2
.word .clip1

.clipper_jumptable:
.word .clip0
.word .clip1
.word .clip2
.word .clip3


@@ void __pimp_mixer_clip_samples(s8 *target, s32 *source, u32 samples, u32 dc_offs);
#define DST       r0
#define SRC       r1
#define COUNT     r2
#define BIAS      r3

#define TEMP1     r4
#define TEMP2     r5

.global __pimp_mixer_clip_samples
__pimp_mixer_clip_samples:
	stmfd sp!, {r4-r12, lr} @ store all registers but parameters and stack	
	
	rsb BIAS, BIAS, #128
	
	@ fixup, jump to the correct position
	and TEMP1, COUNT, #3
	adr	TEMP2, .clipper_jumptable
	mov COUNT, COUNT, lsr #2
	add COUNT, #1
	ldr	pc, [TEMP2, TEMP1, lsl #2]
	
.clip_single_loop:                    @ 10 cycles pr sample:
.clip4:
	ldr   TEMP1, [SRC], #4            @ 3 cycles
	add   TEMP1, BIAS, r4, asr #8     @ 1 cycle
	movs  TEMP2, TEMP1, asr #8        @ 1 cycle
	mvnne TEMP1, TEMP2, lsr #24       @ 1 cycle
	eor   TEMP1, TEMP1, #0x80         @ 1 cycle
	strb  TEMP1, [DST], #1            @ 2 cycles

.clip3:
	ldr   TEMP1, [SRC], #4
	add   TEMP1, BIAS, r4, asr #8
	movs  TEMP2, TEMP1, asr #8
	mvnne TEMP1, TEMP2, lsr #24
	eor   TEMP1, TEMP1, #0x80
	strb  TEMP1, [DST], #1
	
.clip2:
	ldr   TEMP1, [SRC], #4
	add   TEMP1, BIAS, r4, asr #8
	movs  TEMP2, TEMP1, asr #8
	mvnne TEMP1, TEMP2, lsr #24
	eor   TEMP1, TEMP1, #0x80
	strb  TEMP1, [DST], #1
	
.clip1:
	ldr   TEMP1, [SRC], #4
	add   TEMP1, BIAS, r4, asr #8
	movs  TEMP2, TEMP1, asr #8
	mvnne TEMP1, TEMP2, lsr #24
	eor   TEMP1, TEMP1, #0x80
	strb  TEMP1, [DST], #1

.clip0:

	subs COUNT, COUNT, #1             @ 1 cycle
	bne .clip_single_loop             @ 3 cycles

	ldmfd sp!, {r4-r12, lr} @ restore all registers but parameters and stack
	bx lr

	
#define FLIP_SIGN r6
#define TEMP r12

	ldr FLIP_SIGN, =0x80808080
	
.clip_loop:                         @ 7 cycles pr sample:
	ldmia r1!, {r4-r7}              @ 6 cycles
	
	add   r4, BIAS, r4, asr #8      @ 1 cycle
	movs  TEMP, r4, asr #8          @ 1 cycle
	mvnne r4, TEMP, lsr #24         @ 1 cycle
	
	add   r5, BIAS, r4, asr #8
	movs  TEMP, r5, asr #8
	mvnne r5, TEMP, lsr #24
	
	add   r6, BIAS, r4, asr #8
	movs  TEMP, r6, asr #8
	mvnne r6, TEMP, lsr #24
	
	add   r7, BIAS, r4, asr #8
	movs  TEMP, r7, asr #8
	mvnne r7, TEMP, lsr #24
	
	orr   r4, r4, r5, asl #8        @ 1 cycle
	orr   r4, r4, r6, asl #16       @ 1 cycle
	orr   r4, r4, r7, asl #24       @ 1 cycle
	eor   r4, r4, FLIP_SIGN         @ 1 cycle
	str  r4, [r0], #4               @ 2 cycles
	
	subs COUNT, COUNT, #1           @ 1 cycle
	bne .clip_loop                  @ 3 cycles
