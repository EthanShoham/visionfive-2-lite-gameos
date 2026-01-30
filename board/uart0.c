#include "../common/types.h"
#include "mmio.h"
#include "uart0.h"

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

void uart0_hw_init(void) {
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

void uart0_putc(char c) {
  while ((mmio_read32(uart_reg(REG_LSR)) & LSR_THRE) == 0u) 
        ;
  mmio_write32(uart_reg(REG_THR_RBR_DLL), (u32)(u8)c);
}

void uart0_puts(const char *s) {
  for (; *s; s++) {
    char c = *s;
    if (c == '\n') {
      uart0_putc('\r');
    }
    uart0_putc(c);
  }
}

static inline void print_dec(u64 number) {
  char buff[20];
  u32 i = 0;

  if (number == 0) {
    uart0_putc('0');
    return;
  }

  while (number != 0) {
    buff[i++] = (char)('0' + (number % 10));
    number /= 10;
  }

  while (i--) {
    uart0_putc(buff[i]);
  }
}

void print_hex8(u8 v) {
  const char hex[] = "0123456789ABCDEF";
  uart0_putc(hex[(v >> 4) & 0xFu]);
  uart0_putc(hex[v & 0xFu]);
}

void print_hex32(u32 v) {
  uart0_putc('0'); uart0_putc('x');
  for (int i = 3; i >= 0; --i) {
    u8 b = (u8)((v >> (8u * (u32)i)) & 0xFFu);
    print_hex8(b);
  }
}

void print_hex64(u64 v) {
  uart0_putc('0'); uart0_putc('x');
  for (int i = 7; i >= 0; --i) {
    u8 b = (u8)((v >> (8u * (u32)i)) & 0xFFu);  // MSB first
    print_hex8(b);
  }
}

void dump_bytes(const char *label, const u8 *buf, u32 len) {
  uart0_puts(label);
  for (u32 i = 0; i < len; i++) {
    uart0_putc(' ');
    print_hex8(buf[i]);
  }
  uart0_puts("\n");
}
