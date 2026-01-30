.section .text.trap
.globl trap_vector
trap_vector:
    addi sp, sp, -16
    sd t0, 0(sp)
    sd t1, 8(sp)

    csrr t0, scause
    li t1, 0x8000000000000001
    bne t0, t1, trap_return

    csrr t0, sip
    li t1, -3
    and t0, t0, t1
    csrw sip, t0

trap_return:
    ld t0, 0(sp)
    ld t1, 8(sp)
    addi sp, sp, 16
    sret
