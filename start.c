#include "riscv.h"

#define MIL 3000000

#define AON_IOMUX_CFG_BASE 0x17020000
#define AON_GPIO_DOEN AON_IOMUX_CFG_BASE
#define AON_GPIO_DOUT AON_IOMUX_CFG_BASE + 0x4
#define AON_GPIO_ENABLE AON_IOMUX_CFG_BASE + 0xC

#define SYS_CRG_BASE 0x13020000u
#define SYS_IOMUX_BASE 0x13040000u
#define CLK_ENABLE_MASK 0x80000000u
#define UART0_CLK_APB_OFFSET (145u * 4u)
#define UART0_CLK_CORE_OFFSET (146u * 4u)
#define SYSCRG_RESET_ASSERT2 0x300u

#define GPIO_NUM_SHIFT 2
#define GPIO_BYTE_SHIFT 3
#define GPIO_INDEX_MASK 0x3
#define GPIO_DOEN_MASK 0x3f
#define GPIO_DOUT_MASK 0x7f
#define GPIO_DIN_MASK 0x7f

static inline void delay(volatile u64 count) {
  while (count--);
}

static inline u32 mmio_read32(uptr a) { return *(volatile u32 *)a; }

static inline void mmio_write32(uptr a, u32 v) { *(volatile u32 *)a = v; }

static inline void set_bits(uptr a, u32 mask) {
  mmio_write32(a, mmio_read32(a) | mask);
}

static inline void clear_bits(uptr a, u32 mask) {
  mmio_write32(a, mmio_read32(a) & ~mask);
}

#define GPIO_OFFSET(gpio) (((gpio) >> GPIO_NUM_SHIFT) << GPIO_NUM_SHIFT)
#define GPIO_SHIFT(gpio) (((gpio) & GPIO_INDEX_MASK) << GPIO_BYTE_SHIFT)

static inline void sys_iomux_doen(u32 gpio, u32 oen) {
  uptr addr = SYS_IOMUX_BASE + GPIO_OFFSET(gpio);
  u32 mask = GPIO_DOEN_MASK << GPIO_SHIFT(gpio);
  u32 val = (oen & GPIO_DOEN_MASK) << GPIO_SHIFT(gpio);
  mmio_write32(addr, (mmio_read32(addr) & ~mask) | val);
}

static inline void sys_iomux_dout(u32 gpio, u32 gpo) {
  uptr addr = SYS_IOMUX_BASE + 0x40 + GPIO_OFFSET(gpio);
  u32 mask = GPIO_DOUT_MASK << GPIO_SHIFT(gpio);
  u32 val = (gpo & GPIO_DOUT_MASK) << GPIO_SHIFT(gpio);
  mmio_write32(addr, (mmio_read32(addr) & ~mask) | val);
}

static inline void sys_iomux_din(u32 gpio, u32 gpi) {
  uptr addr = SYS_IOMUX_BASE + 0x80 + GPIO_OFFSET(gpi);
  u32 mask = GPIO_DIN_MASK << GPIO_SHIFT(gpi);
  u32 val = ((gpio + 2) & GPIO_DIN_MASK) << GPIO_SHIFT(gpi);
  mmio_write32(addr, (mmio_read32(addr) & ~mask) | val);
}

/* 16550 register indices */
enum {
  REG_THR_RBR_DLL = 0, /* THR (W), RBR (R), DLL when DLAB=1 */
  REG_IER_DLM = 1,     /* IER, DLM when DLAB=1 */
  REG_IIR_FCR = 2,     /* IIR (R), FCR (W) */
  REG_LCR = 3,         /* Line Control */
  REG_MCR = 4,         /* Modem Control */
  REG_LSR = 5,         /* Line Status */
};

#define LCR_DLAB 0x80u
#define LCR_8N1 0x03u
#define FCR_EN 0x01u
#define FCR_RXRST 0x02u
#define FCR_TXRST 0x04u
#define MCR_DTR 0x01u
#define MCR_RTS 0x02u

#define UART0_BASE 0x10000000
#define UART_REG_SHIFT 2u

static inline uptr uart_reg(u32 reg_index) {
  return (uptr)(UART0_BASE + ((uptr)reg_index << UART_REG_SHIFT));
}

static void uart0_init(u32 uart_clk_hz, u32 baud) {
  u32 div = (uart_clk_hz + (8u * baud)) / (16u * baud);

  mmio_write32(uart_reg(REG_IER_DLM), 0u);                 /* disable IRQs */
  mmio_write32(uart_reg(REG_LCR), LCR_DLAB | LCR_8N1);     /* DLAB=1, 8N1 */
  mmio_write32(uart_reg(REG_THR_RBR_DLL), div & 0xFFu);    /* DLL */
  mmio_write32(uart_reg(REG_IER_DLM), (div >> 8) & 0xFFu); /* DLM */
  mmio_write32(uart_reg(REG_LCR), LCR_8N1);                /* DLAB=0, 8N1 */
  mmio_write32(uart_reg(REG_MCR), MCR_DTR | MCR_RTS);
  mmio_write32(uart_reg(REG_IIR_FCR), FCR_EN | FCR_RXRST | FCR_TXRST);
}

static void uart0_hw_init(void) {
  set_bits(SYS_CRG_BASE + UART0_CLK_APB_OFFSET, CLK_ENABLE_MASK);
  set_bits(SYS_CRG_BASE + UART0_CLK_CORE_OFFSET, CLK_ENABLE_MASK);
  clear_bits(SYS_CRG_BASE + SYSCRG_RESET_ASSERT2, (1u << 19) | (1u << 20));
  sys_iomux_doen(5, 0);
  sys_iomux_dout(5, 20);
  sys_iomux_doen(6, 1);
  sys_iomux_din(6, 14);
  uart0_init(24000000u, 115200u);
}

#define LSR_THRE 0x20u /* LSR[5] THR empty */

static void uart0_putc(char c) {
  while ((mmio_read32(uart_reg(REG_LSR)) & LSR_THRE) == 0u) {
  }
  mmio_write32(uart_reg(REG_THR_RBR_DLL), (u32)(u8)c);
}

static void uart0_puts(const char *s) {
  for (; *s; s++) {
    char c = *s;
    if (c == '\n') {
      uart0_putc('\r');
    }
    uart0_putc(c);
  }
}

static int cpuid() {
  int id = read_tp_reg();
  return id;
}

#define NCPU 5

__attribute__ ((aligned (16))) u8 stack0[4096 * NCPU];

void start() {
  int id = read_mhartid();
  write_tp_reg(id);

  if (id == 0) {
    uart0_hw_init();
    // Enable AON GPIOs
    mmio_write32(AON_GPIO_ENABLE, 1);

    // Enable output for AON GPIO3
    mmio_write32(AON_GPIO_DOEN, mmio_read32(AON_GPIO_DOEN) & 0xF8FFFFFF);

    uart0_puts("Hello World!\n");

    uart0_putc(id + 48);
    for (;;) {
      // Set AON GPIO3 as high
      mmio_write32(AON_GPIO_DOUT,
                   (mmio_read32(AON_GPIO_DOUT) & 0xF0FFFFFF) | 0x01000000);

      delay(MIL);

      // Set AON GPIO3 as low
      mmio_write32(AON_GPIO_DOUT, mmio_read32(AON_GPIO_DOUT) & 0xF0FFFFFF);

      delay(MIL);
    }
  } else {
    for(;;);
  }
}
