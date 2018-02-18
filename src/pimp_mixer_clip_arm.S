/* pimp_mixer_clip_arm.S -- ARM optimized sample clipping code
 * Copyright (C) 2005-2008 Jørn Nystad and Erik Faye-Lund
 * For conditions of distribution and use, see copyright notice in LICENSE.TXT
 */

.text
.section .iwram
.arm
.align 2

@@ void pimp_mixer_clip_samples(s8 *target, s32 *source, u32 samples, u32 dc_offs);
#define DST       r0
#define SRC       r1
#define COUNT     r2
#define BIAS      r3

#define S1        r4
#define S2        r5
#define S3        r6
#define S4        r7

#define CONST_00  r8
#define CONST_FF  r9

#define FLIP_SIGN r10
#define TEMP      r11

.global pimp_mixer_clip_samples
.type pimp_mixer_clip_samples, %function
pimp_mixer_clip_samples:
	stmfd sp!, {r4-r12, lr} @ store all registers but parameters and stack
	
	ldr FLIP_SIGN, =0x80808080
	ldr CONST_00, =0
	ldr CONST_FF, =0xFF
	rsb BIAS, BIAS, #128
	
	cmp COUNT, #7
	bmi .fixup_last
	
	@ fixup first, jump to the correct position
	@ TEMP = 3 - ((DST - 1) & 3)
	sub  TEMP, DST, #1
	and  TEMP, TEMP, #3
	rsb  TEMP, TEMP, #3
	
	cmp  TEMP, #0
	beq  .no_fixup_first
	
	@ update COUNT
	sub COUNT, COUNT, TEMP
	
	@ clip a single sample pr iteration
.fixup_first_loop:                        @ 10-13 cycles pr sample:
	ldr   S1, [SRC], #4                   @ 3 cycles
	add   S1, BIAS, S1, asr #8            @ 1 cycle
	subs  r12, CONST_00, S1, asr #8       @ 1 cycle
	andne S1, CONST_FF, r12, lsr #24      @ 1 cycle
	eor   S1, S1, #0x80                   @ 1 cycle
	strb  S1, [DST], #1                   @ 2 cycles
	
	@ loop
	subs TEMP, TEMP, #1                   @ 1 cycle
	bne .fixup_first_loop                 @ 3 cycles
.no_fixup_first:
	
	sub COUNT, COUNT, #4
	
.clip_loop:                               @ 7 cycles pr sample:
	ldmia SRC!, {S1-S4}                   @ 6 cycles
	
	add   S1, BIAS, S1, asr #8            @ 1 cycle
	subs  TEMP, CONST_00, S1, asr #8      @ 1 cycle
	andne S1, CONST_FF, TEMP, lsr #24     @ 1 cycle
	
	add   S2, BIAS, S2, asr #8            @ 1 cycle
	subs  TEMP, CONST_00, S2, asr #8      @ 1 cycle
	andne S2, CONST_FF, TEMP, lsr #24     @ 1 cycle
	
	add   S3, BIAS, S3, asr #8            @ 1 cycle
	subs  TEMP, CONST_00, S3, asr #8      @ 1 cycle
	andne S3, CONST_FF, TEMP, lsr #24     @ 1 cycle
	
	add   S4, BIAS, S4, asr #8            @ 1 cycle
	subs  TEMP, CONST_00, S4, asr #8      @ 1 cycle
	andne S4, CONST_FF, TEMP, lsr #24     @ 1 cycle
	
	orr   S1, S1, S2, lsl #8              @ 1 cycle
	orr   S1, S1, S3, lsl #16             @ 1 cycle
	orr   S1, S1, S4, lsl #24             @ 1 cycle
	eor   S1, S1, FLIP_SIGN               @ 1 cycle
	str   S1, [DST], #4                   @ 2 cycles
	
	subs COUNT, COUNT, #4                 @ 1 cycle
	bpl .clip_loop                        @ 3 cycles
	                                      @ total: 28 cycles
	                                      @ 28 cycles / 4 samples = 7 cycles per sample
	
	add COUNT, COUNT, #4
	
.fixup_last:
	cmp COUNT, #0
	beq .no_fixup_last
	
	@ clip a single sample pr iteration
.fixup_last_loop:                         @ 10-13 cycles pr sample:
	ldr   S1, [SRC], #4                   @ 3 cycles
	add   S1, BIAS, S1, asr #8            @ 1 cycle
	subs  r12, CONST_00, S1, asr #8       @ 1 cycle
	andne S1, CONST_FF, r12, lsr #24      @ 1 cycle
	eor   S1, S1, #0x80                   @ 1 cycle
	strb  S1, [DST], #1                   @ 2 cycles
	
	@ loop
	subs COUNT, COUNT, #1                 @ 1 cycle
	bne .fixup_last_loop                  @ 3 cycles
.no_fixup_last:
	
	ldmfd sp!, {r4-r12, lr} @ restore all registers but parameters and stack
	bx lr
