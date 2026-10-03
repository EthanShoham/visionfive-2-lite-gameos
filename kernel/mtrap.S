.section .text.mtrap
.globl mtrap_vector
mtrap_vector:
    addi sp, sp, -24
    sd t0, 0(sp)
    sd t1, 8(sp)
    sd t2, 16(sp)

    csrr t0, mcause
    li t1, 0x8000000000000007
    bne t0, t1, mtrap_return

    li t0, 0x2000000
    li t1, 0xBFF8
    add t0, t0, t1
    ld t0, 0(t0)
    li t1, 1000000
    add t0, t0, t1

    li t1, 0x2000000
    li t2, 0x4000
    add t1, t1, t2
    csrr t2, mhartid
    slli t2, t2, 3
    add t1, t1, t2
    sd t0, 0(t1)

    csrr t0, mip
    ori t0, t0, 2
    csrw mip, t0

mtrap_return:
    ld t0, 0(sp)
    ld t1, 8(sp)
    ld t2, 16(sp)
    addi sp, sp, 24
    mret
