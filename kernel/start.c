#include "param.h"
#include "riscv.h"
#include "../common/types.h"

int main();
void timerinit();
void boardinit(u32 cpu_id);

__attribute__((aligned(16))) u8 stack0[4096 * NCPU];

// entry.S jumps here in machine mode on stack0 per cpu hart
void start() {
  // keep each CPU's hartid in its tp register, for cpuid().
  i32 id = read_mhartid_csr();
  write_tp_reg(id);

  boardinit(id);

  // set M Previous Privilege mode to Supervisor, for mret.
  u64 x = read_mstatus_csr();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  write_mstatus_csr(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  write_mepc_csr((u64)main);

  // disable paging for now.
  write_satp_csr(0);

  // delegate all interrupts and exceptions to supervisor mode.
  write_medeleg_csr(0xffff);
  write_mideleg_csr(0xffff);
  write_sie_csr(read_sie_csr() | SIE_SEIE | SIE_STIE);

  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  write_pmpaddr0_csr(0x3fffffffffffffull);
  write_pmpcfg0_csr(0xf);

  // ask for clock interrupts.
  timerinit();

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
