#include "../common/types.h"
#include "param.h"
#include "riscv.h"

int main();
void timerinit();
void boardinit(u32 cpu_id);
void uart0_hw_init();
void uart0_putc(char c);

__attribute__((aligned(16))) u8 stack0[4096 * NCPU];

volatile b32 uartinit = false;
// entry.S jumps here in machine mode on stack0 per cpu hart
void start() {
  uartinit = false;
  // keep each CPU's hartid in its tp register, for cpuid().
  i32 id = read_mhartid_csr();
  write_tp_reg(id);

  if (id == 0) {
    uart0_hw_init();
    uart0_putc('t');
    uart0_putc(id + 48);
    boardinit(id);
    uartinit = true;
  } else {
    while (!uartinit)
      ;
    __sync_synchronize();
  }

  uart0_putc('a');
  // set M Previous Privilege mode to Supervisor, for mret.
  u64 x = read_mstatus_csr();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  write_mstatus_csr(x);

  uart0_putc('b');
  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  write_mepc_csr((u64)main);

  uart0_putc('c');

  if(id == 0) {
    for(;;)
      ;
  }

  // disable paging for now.
  write_satp_csr(0);

  uart0_putc('d');
  // delegate all interrupts and exceptions to supervisor mode.
  write_medeleg_csr(0xffff);
  write_mideleg_csr(0xffff);
  write_sie_csr(read_sie_csr() | SIE_SEIE | SIE_STIE);

  uart0_putc('e');
  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  write_pmpaddr0_csr(0x3fffffffffffffull);
  write_pmpcfg0_csr(0xf);

  uart0_putc('f');
  // ask for clock interrupts.
  // timerinit();

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

// ask each hart to generate timer interrupts.
void timerinit() {
  // enable supervisor-mode timer interrupts.
  write_mie_csr(read_mie_csr() | MIE_STIE);

  // enable the sstc extension (i.e. stimecmp).
  write_menvcfg_csr(read_menvcfg_csr() | (1LL << 63));

  // allow supervisor to use stimecmp and time.
  write_mcounteren_csr(read_mcounteren_csr() | 2);

  // ask for the very first timer interrupt.
  write_stimecmp_csr(read_time_csr() + 1000000);
}
