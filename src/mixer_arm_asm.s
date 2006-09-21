.text
.section .iwram
.arm
.align 2

@ no mixing:               406517
@ without bresenham:
@ inline-asm version:     1211037
@ current:                1192545
@ with bresenham:
@ inline-asm version:     1163062 (718741)
@ current:                1146840 (702519)


#define NO_MIXING
#define USE_BRESENHAM_MIXER

.stack_store:
.word 0

.jumptable:
.word .mix0
.word .mix1
.word .mix2
.word .mix3
.word .mix4
.word .mix5
.word .mix6
.word .mix7

.global __pimp_mixer_mix_samples_asm
__pimp_mixer_mix_samples_asm:
	stmfd sp!, {r4-r12, lr} @ store all registers but parameters and stack

#ifdef NO_MIXING
	ldmfd sp!, {r4-r12, lr} @ restore rest of registers
	bx lr                   @ return to caller
#endif

	str sp, .stack_store    @ store stack pointer so we can use that register in our mixer (note, interrupts must be disabled for this to be safe)

#define UNROLL_RANGE        r4-r11
#define TARGET              r0
#define COUNTER             r1
#define SAMPLE_DATA         r2
#define VOLUME              r3
#define SAMPLE_CURSOR       lr
#define SAMPLE_CURSOR_DELTA r12

	@ load rest of parameters
	ldr SAMPLE_CURSOR, [sp, #40]
	ldr SAMPLE_CURSOR_DELTA, [sp, #44]
	
	@ find how many samples to fixup
	and r4, COUNTER, #7

	@ fixup, jump to the correct position
	adr	r5, .jumptable
	ldr	pc, [r5, r4, lsl #2]
	
	@ fixup code unrolled 7 times
.mix7:
	ldr   r6, [TARGET]
	ldrb  r5, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r6, r5, VOLUME, r6
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	str   r6, [TARGET], #4

.mix6:
	ldr   r6, [TARGET]
	ldrb  r5, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r6, r5, VOLUME, r6
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	str   r6, [TARGET], #4

.mix5:
	ldr   r6, [TARGET]
	ldrb  r5, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r6, r5, VOLUME, r6
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	str   r6, [TARGET], #4

.mix4:
	ldr   r6, [TARGET]
	ldrb  r5, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r6, r5, VOLUME, r6
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	str   r6, [TARGET], #4

.mix3:
	ldr   r6, [TARGET]
	ldrb  r5, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r6, r5, VOLUME, r6
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	str   r6, [TARGET], #4

.mix2:
	ldr   r6, [TARGET]
	ldrb  r5, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r6, r5, VOLUME, r6
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	str   r6, [TARGET], #4

.mix1:
	ldr   r6, [TARGET]
	ldrb  r5, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r6, r5, VOLUME, r6
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	str   r6, [TARGET], #4

.mix0:
	
	movs COUNTER, COUNTER, asr #3 @ divide counter by 8
	beq .ret                      @ if no more samples, return
	
	
// if (sample_cursor_delta > 0 && sample_cursor_delta < (1UL << 12))
// this can be done as:
// if ((sample_cursor_delta & ~((1UL << 12) - 1)) == 0)

	@ check if bresenham mixer can be used or not 

#ifdef USE_BRESENHAM_MIXER
	ldr r4, =0xfffff000
	tst SAMPLE_CURSOR_DELTA, r4
	beq .bresenham_mixer
#endif

#define TEMP                sp

.simple_loop:
	ldmia TARGET, {UNROLL_RANGE}
	
	ldrb  TEMP, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r4, TEMP, VOLUME, r4
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA

	ldrb  TEMP, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r5, TEMP, VOLUME, r5
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA

	ldrb  TEMP, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r6, TEMP, VOLUME, r6
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA

	ldrb  TEMP, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r7, TEMP, VOLUME, r7
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	
	ldrb  TEMP, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r8, TEMP, VOLUME, r8
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA

	ldrb  TEMP, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r9, TEMP, VOLUME, r9
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA

	ldrb  TEMP, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r10, TEMP, VOLUME, r10
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA

	ldrb  TEMP, [SAMPLE_DATA, SAMPLE_CURSOR, lsr #12]
	mla   r11, TEMP, VOLUME, r11
	add   SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	
	stmia TARGET!, {UNROLL_RANGE}
	subs COUNTER, COUNTER, #1
	bne .simple_loop


.ret:
	@ clean return
	mov r0, SAMPLE_CURSOR

	ldr sp, .stack_store    @ restore stack pointer
	ldmfd sp!, {r4-r12, lr} @ restore rest of registers
	bx lr                   @ return to caller

#ifdef USE_BRESENHAM_MIXER

.bresenham_mixer:
	str SAMPLE_DATA, [sp] @ stash away SAMPLE_DATA for later use
	add SAMPLE_DATA, SAMPLE_DATA, SAMPLE_CURSOR, lsr #12
	
	mov SAMPLE_CURSOR, SAMPLE_CURSOR, asl #20
	mov SAMPLE_CURSOR_DELTA, SAMPLE_CURSOR_DELTA, asl #20

	ldrb  TEMP, [SAMPLE_DATA], #1
	mul   TEMP, VOLUME, sp
.bresenham_loop:
	ldmia TARGET, {r4-r11}

	add   r4, TEMP, r4
	adds  SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	ldrcsb TEMP, [SAMPLE_DATA], #1
	mulcs  TEMP, VOLUME, TEMP

	add   r5, TEMP, r5
	adds  SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	ldrcsb TEMP, [SAMPLE_DATA], #1
	mulcs  TEMP, VOLUME, TEMP

	add   r6, TEMP, r6
	adds  SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	ldrcsb TEMP, [SAMPLE_DATA], #1
	mulcs  TEMP, VOLUME, TEMP

	add   r7, TEMP, r7
	adds  SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	ldrcsb TEMP, [SAMPLE_DATA], #1
	mulcs  TEMP, VOLUME, TEMP

	add   r8, TEMP, r8
	adds  SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	ldrcsb TEMP, [SAMPLE_DATA], #1
	mulcs  TEMP, VOLUME, TEMP

	add   r9, TEMP, r9
	adds  SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	ldrcsb TEMP, [SAMPLE_DATA], #1
	mulcs  TEMP, VOLUME, TEMP

	add   r10, TEMP, r10
	adds  SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	ldrcsb TEMP, [SAMPLE_DATA], #1
	mulcs  TEMP, VOLUME, TEMP

	add   r11, TEMP, r11
	adds  SAMPLE_CURSOR, SAMPLE_CURSOR, SAMPLE_CURSOR_DELTA
	ldrcsb TEMP, [SAMPLE_DATA], #1
	mulcs  TEMP, VOLUME, TEMP

	stmia TARGET!, {r4-r11}
	subs  COUNTER, COUNTER, #1
	bne .bresenham_loop
	
	ldr sp, .stack_store    @ restore stack pointer

	ldr r0, [sp]            @ restore the old sample data
	
	@ calculate how the sample cursor changed
	sub r0, SAMPLE_DATA, r0
	sub r0, r0, #1
	mov r0, r0, lsl #12
	add r0, SAMPLE_CURSOR, asr #20
	
	@ return to caller
	ldmfd sp!, {r4-r12, lr} @ restore rest of registers
	bx lr

#endif
