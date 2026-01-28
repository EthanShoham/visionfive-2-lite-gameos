#ifndef RISCV
#define RISCV

#ifndef __ASSEMBLER__
#include "types.h"

// which hart (core) is this?
static inline u64 read_mhartid() {
  u64 x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

// read and write tp, the thread pointer, which xv6 uses to hold
// this core's hartid (core number), the index into cpus[].
static inline u64 read_tp_reg() {
  u64 x;
  asm volatile("mv %0, tp" : "=r" (x) );
  return x;
}

static inline void write_tp_reg(u64 x) {
  asm volatile("mv tp, %0" : : "r" (x));
}

#endif // __ASSEMBLER__

#define PGSIZE 4096

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
#endif
