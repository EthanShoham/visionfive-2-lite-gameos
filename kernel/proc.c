#include "proc.h"
#include "param.h"
#include "defs.h"
#include "riscv.h"

struct cpu cpus[NCPU];

i32 cpuid(void) {
  i32 id = read_tp_reg();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu* mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}
