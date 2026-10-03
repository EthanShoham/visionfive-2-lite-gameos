#include "../common/types.h"
#include "../board/uart0.h"
#include "param.h"
#include "riscv.h"

int main();
void timerinit();
void trap_vector();
void mtrap_vector();

__attribute__((aligned(16))) u8 stack0[4096 * NCPU];

// entry.S jumps here in machine mode on stack0 per cpu hart
void start() {
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

  // disable paging for now.
  write_satp_csr(0);

  uart0_putc('d');
  // delegate all interrupts and exceptions to supervisor mode.
  write_medeleg_csr(0xffff);
  write_mideleg_csr(0xffff);
  write_sie_csr(read_sie_csr() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  uart0_putc('e');
  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  write_pmpaddr0_csr(0x3fffffffffffffull);
  write_pmpcfg0_csr(0xf);

  uart0_putc('f');
  // set supervisor trap vector before enabling timer interrupts.
  w_stvec((u64)trap_vector);
  write_mtvec_csr((u64)mtrap_vector);

  // ask for clock interrupts.
  timerinit();

  uart0_putc('g');
  // keep each CPU's hartid in its tp register, for cpuid().
  i32 id = read_mhartid_csr();
  write_tp_reg(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

// ask each hart to generate timer interrupts.
void timerinit() {
  // Allow S-mode to read time.
  write_mcounteren_csr(read_mcounteren_csr() | 2);

  // Program CLINT mtimecmp and enable machine timer interrupts.
  uart0_putc('1');
  const u64 CLINT_BASE = 0x2000000ull;
  const u64 MTIMECMP_BASE = CLINT_BASE + 0x4000ull;
  const u64 MTIME = CLINT_BASE + 0xBFF8ull;
  u64 hart = read_mhartid_csr();
  volatile u64 *mtimecmp = (volatile u64 *)(MTIMECMP_BASE + (hart * 8ull));
  volatile u64 *mtime = (volatile u64 *)MTIME;
  *mtimecmp = *mtime + 1000000ull;

  uart0_putc('2');
  write_mie_csr(read_mie_csr() | MIE_MTIE);

  uart0_putc('3');
  write_mstatus_csr(read_mstatus_csr() | MSTATUS_MIE);
  uart0_putc('4');
}
