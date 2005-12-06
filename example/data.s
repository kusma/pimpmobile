.section .rodata

.macro datafile symbol, filename
.global \symbol
.align
\symbol:
.incbin "\filename"
.endm

datafile sample, example/sample.raw
.global sample_end
sample_end:
