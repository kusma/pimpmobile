.section .rodata

.macro datafile symbol, filename
.global \symbol
.align
\symbol:
.incbin "\filename"
.endm

datafile sample_bank, sample_bank.bin
datafile module, rhino.mod.bin
