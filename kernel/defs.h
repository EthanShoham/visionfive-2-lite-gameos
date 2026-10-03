#ifndef DEFS_H
#define DEFS_H
#include "../common/types.h"

struct spinlock;
struct cpu;

// proc.c
i32 cpuid(void);
struct cpu*     mycpu(void);

// spinlock.c
void            acquire(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);

#endif
