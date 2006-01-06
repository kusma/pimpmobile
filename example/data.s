.section .rodata

.macro datafile symbol, filename
.global \symbol
.align
\symbol:
.incbin "\filename"
.endm

datafile sample_bank, sample_bank.bin
.global sample_bank_end
sample_bank_end:

datafile module, rhino.bin
