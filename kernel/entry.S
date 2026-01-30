.section .text.entry
.globl _entry
_entry:
    la sp, stack0
    li a0, 1024*4
    csrr a1, mhartid
    // addi a1, a1, 1 // U74 cores hartid are 1,2,3,4
    mul a0, a0, a1
    add sp, sp, a0
    call start
spin:
    j spin
