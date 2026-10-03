#include "ack_led.h"
#include "mmio.h"

void ack_led_init(void) {
    // Enable AON GPIOs
    mmio_write32(AON_GPIO_ENABLE, 1);

    // Enable output for AON GPIO3
    mmio_write32(AON_GPIO_DOEN, mmio_read32(AON_GPIO_DOEN) & 0xF8FFFFFF);
}

void ack_led_on(void) {
         // Set AON GPIO3 as high
      mmio_write32(AON_GPIO_DOUT,
                   (mmio_read32(AON_GPIO_DOUT) & 0xF0FFFFFF) | 0x01000000);
}

void ack_led_off(void) {
      // Set AON GPIO3 as low
      mmio_write32(AON_GPIO_DOUT, mmio_read32(AON_GPIO_DOUT) & 0xF0FFFFFF);
}
