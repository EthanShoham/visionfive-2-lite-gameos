#ifndef SPINLOCK_H
#define SPINLOCK_H
#include "../common/types.h"
#include "proc.h"

struct spinlock {
  u32 locked;

  char *name;
  struct cpu *cpu;
};

#endif
