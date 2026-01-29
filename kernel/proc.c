#include "defs.h"

i32 cpuid(void) {
  i32 id = read_tp_reg();
  return id;
}
