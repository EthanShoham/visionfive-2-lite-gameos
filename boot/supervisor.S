#define DDR_BOOT 0x40000000

.section .text.supervisor
.globl _supervisor
_supervisor:
    la sp, stack_top
    csrr t0, mhartid
    bnez t0, spin // only hart0 should get here
    call supervisor_start // this should not return
spin:
    j spin
