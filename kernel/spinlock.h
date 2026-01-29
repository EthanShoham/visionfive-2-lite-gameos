#ifndef __SPINLOCK__
#define __SPINLOCK__
#include "../common/types.h"
#include "proc.h"

struct spinlock {
  u32 locked;

  char *name;
  struct cpu *cpu;
};

#endif
