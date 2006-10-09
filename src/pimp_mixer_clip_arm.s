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
#define DST   r0
#define SRC   r1
#define COUNT r2
#define BIAS  r3

.global __pimp_mixer_clip_samples
__pimp_mixer_clip_samples:
	stmfd sp!, {r4-r12, lr} @ store all registers but parameters and stack	
	
	@ fixup, jump to the correct position
	and r4, COUNT, #3
	adr	r5, .clipper_jumptable
	ldr	pc, [r5, r4, lsl #2]
	
clip_single_loop: @ 8.5 cycles pr sample 
clip3:
	ldr r4, [SRC], #4              @ 3 cycles
	add   r4, BIAS, r4, asr #8     @ 1 cycle
	movs  TEMP, r4, asr #8         @ 1 cycle
	mvnnz r4, TEMP, lsr #24        @ 1 cycles
	strb r4, [DST], #1             @ 2 cycles
	
clip2:
	ldr r4, [SRC], #4
	add   r4, BIAS, r4, asr #8
	movs  TEMP, r4, asr #8
	mvnnz r4, TEMP, lsr #24
	strb r4, [DST], #1
	
clip1:
	ldr r4, [SRC], #4
	add   r4, BIAS, r4, asr #8
	movs  TEMP, r4, asr #8
	mvnnz r4, TEMP, lsr #24
	strb r4, [DST], #1
	
clip0:
	
	subs COUNT, COUNT, #1
	bne .clip_single_loop

	bx lr
	
	
	
	
.clip_loop: @ 6.5 cycles pr sample 
	ldmia r1!, {r4-r7}              @ 6 cycles
	
	add   r4, BIAS, r4, asr #8     @ 1 cycle
	movs  TEMP, r4, asr #8         @ 1 cycle
	mvnnz r4, TEMP, lsr #24        @ 1 cycle
	
	add   r5, BIAS, r4, asr #8
	movs  TEMP, r5, asr #8
	mvnnz r5, TEMP, lsr #24
	
	add   r6, BIAS, r4, asr #8
	movs  TEMP, r6, asr #8
	mvnnz r6, TEMP, lsr #24
	
	add   r7, BIAS, r4, asr #8
	movs  TEMP, r7, asr #8
	mvnnz r7, TEMP, lsr #24
	
	orr   r4, r4, r5, asl #8      @ 1 cycle
	orr   r4, r4, r6, asl #16     @ 1 cycle
	orr   r4, r4, r7, asl #24    @ 1 cycle
	eor   r4, r4, FLIP_SIGN       @ 1 cycle
	str  r4, [r0], #4           @ 2 cycles
	
	subs COUNT, COUNT, #1
	bne .clip_loop
	