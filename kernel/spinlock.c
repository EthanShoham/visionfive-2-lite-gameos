#include "spinlock.h"
#include "defs.h"

void initlock(struct spinlock *lk, char *name) {
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

void acquire(struct spinlock *lk) {
  while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
    ;

  __sync_synchronize();

  lk->cpu = mycpu();
}

void release(struct spinlock *lk) {
  lk->cpu = 0;

  // Tell the C compiler and the CPU to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released,
  // and that loads in the critical section occur strictly before
  // the lock is released.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code doesn't use a C assignment, since the C standard
  // implies that an assignment might be implemented with
  // multiple store instructions.
  // On RISC-V, sync_lock_release turns into an atomic swap:
  //   s1 = &lk->locked
  //   amoswap.w zero, zero, (s1)
  __sync_lock_release(&lk->locked);
}
