#ifndef MMIO_H
#define MMIO_H
#include "../common/types.h"

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

#endif
