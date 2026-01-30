#include "../board/uart0.h"
#include "defs.h"

void main(void) {
  i32 id = cpuid();
  for (;;) {
    uart0_putc('0' + id);
  }
}
