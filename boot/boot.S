#define DDR_BOOT 0x40000000
.section .bss
.align 3            # 8-byte alignment
.globl handoff_supervisor
handoff_supervisor:
.quad 0

.globl handoff_kernel
handoff_kernel:
.quad 0

.section .text.boot
.globl _boot
_boot:
    la sp, stack_top
    csrr t0, mhartid
    bnez t0, wait
    call boot_start

    la t0, handoff_supervisor
    ld t0, 0(t0)
    fence.i
    jalr zero, 0(t0) // jump to supervisor start at DDR

wait:
    la t0, handoff_kernel
    ld t0, 0(t0)
    beqz t0, wait
    fence.i
    jalr zero, 0(t0) // jump to kernel start at DDR
