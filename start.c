#define MIL 3000000

#define AON_IOMUX_CFG_BASE 0x17020000
#define AON_GPIO_DOEN AON_IOMUX_CFG_BASE
#define AON_GPIO_DOUT AON_IOMUX_CFG_BASE + 0x4
#define AON_GPIO_ENABLE AON_IOMUX_CFG_BASE + 0xC

#define MMIO32(addr) (*(volatile unsigned int *)(addr))

static inline void delay(volatile unsigned long count) {
    while(count--){}
}

void start() {
  // Enable AON GPIOs
  MMIO32(AON_GPIO_ENABLE) = 1;

  // Enable output for AON GPIO3
  MMIO32(AON_GPIO_DOEN) = MMIO32(AON_GPIO_DOEN) & 0xF8FFFFFF;

  for (;;) {
    // Set AON GPIO3 as high
    MMIO32(AON_GPIO_DOUT) = (MMIO32(AON_GPIO_DOUT) & 0xF0FFFFFF) | 0x01000000;

    delay(MIL);

    // Set AON GPIO3 as low
    MMIO32(AON_GPIO_DOUT) = (MMIO32(AON_GPIO_DOUT) & 0xF0FFFFFF);

    delay(MIL);
  }
}
