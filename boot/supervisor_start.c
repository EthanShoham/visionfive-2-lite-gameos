#include "../board/ack_led.h"
#include "../common/types.h"

void delay(u32 millsec) {
  // assuming core clock at 1GHz
  volatile u64 clocks = (1ull << 20) * millsec;
  while (clocks--)
    ;
}

// this only runs on hart0 the S7 core
void supervisor_start(void) {
  // first indicator for good
  ack_led_init();

  for (;;) {
    ack_led_on();
    delay(10);
    ack_led_off();
    delay(10);
  }
}
