#ifndef __PROC__
#define __PROC__
#include "../common/types.h"

struct cpu {
  i32 push_off_count;
  i32 interrupts_enabled;
};

#endif
