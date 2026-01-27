.section .text
.globl _start
_start:
      # AON GPIO enable
      li t0, 0x1702000c
      li t1, 1
      sw t1, 0(t0)

      # AON DOEN (output enable) for pin 3: clear bits [26:24]
      li t0, 0x17020000
      lw t1, 0(t0)
      li t2, 0xF8FFFFFF      # ~0x07000000
      and t1, t1, t2
      sw t1, 0(t0)

up:
      # AON DOUT for pin 3: set to 1 in bits [27:24]
      li t0, 0x17020004
      lw t1, 0(t0)
      li t2, 0xF0FFFFFF      # ~0x0F000000
      and t1, t1, t2
      li t3, 0x01000000
      or t1, t1, t3
      sw t1, 0(t0)

      li t4, 0x01817C80
up_loop:
      addi t4, t4, -1
      bne t4, x0, up_loop
down:
      # AON DOUT for pin 3: set to 0 in bits [27:24]
      li t0, 0x17020004
      lw t1, 0(t0)
      li t2, 0xF0FFFFFF      # ~0x0F000000
      and t1, t1, t2
      sw t1, 0(t0)

      li t4, 0x01817C80
down_loop:
      addi t4, t4, -1
      bne t4, x0, down_loop

      beq x0, x0, up
  1:  j 1b
