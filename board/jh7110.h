#ifndef __JH7110__
#define __JH7110__
#include "../common/types.h"

#define SYS_SYSCON_BASE 0x13030000u

// Bit helpers
#ifndef BIT
#define BIT(n) (1u << (n))
#endif
#ifndef GENMASK
#define GENMASK(h, l) (((~0u) << (l)) & (~0u >> (31 - (h))))
#endif

// PLL0 field masks/offsets
#define PLL0_DACPD_MASK BIT(24)
#define PLL0_DSMPD_MASK BIT(25)
#define PLL0_FBDIV_MASK GENMASK(11, 0)
#define PLL0_PD_MASK BIT(27)
#define PLL0_POSTDIV1_MASK GENMASK(29, 28)
#define PLL0_PREDIV_MASK GENMASK(5, 0)

#define PLL0_DACPD_OFFSET 0x18
#define PLL0_DSMPD_OFFSET 0x18
#define PLL0_FBDIV_OFFSET 0x1C
#define PLL0_PD_OFFSET 0x20
#define PLL0_POSTDIV1_OFFSET 0x20
#define PLL0_PREDIV_OFFSET 0x24

// PLL1 field masks/offsets
#define PLL1_DACPD_MASK BIT(15)
#define PLL1_DSMPD_MASK BIT(16)
#define PLL1_FBDIV_MASK GENMASK(28, 17)
#define PLL1_PD_MASK BIT(27)
#define PLL1_POSTDIV1_MASK GENMASK(29, 28)
#define PLL1_PREDIV_MASK GENMASK(5, 0)

#define PLL1_DACPD_OFFSET 0x24
#define PLL1_DSMPD_OFFSET 0x24
#define PLL1_FBDIV_OFFSET 0x24
#define PLL1_PD_OFFSET 0x28
#define PLL1_POSTDIV1_OFFSET 0x28
#define PLL1_PREDIV_OFFSET 0x2c

// PLL2 field masks/offsets
#define PLL2_DACPD_MASK BIT(15)
#define PLL2_DSMPD_MASK BIT(16)
#define PLL2_FBDIV_MASK GENMASK(28, 17)
#define PLL2_PD_MASK BIT(27)
#define PLL2_POSTDIV1_MASK GENMASK(29, 28)
#define PLL2_PREDIV_MASK GENMASK(5, 0)

#define PLL2_DACPD_OFFSET 0x2C
#define PLL2_DSMPD_OFFSET 0x2C
#define PLL2_FBDIV_OFFSET 0x2C
#define PLL2_PD_OFFSET 0x30
#define PLL2_POSTDIV1_OFFSET 0x30
#define PLL2_PREDIV_OFFSET 0x34

#define PLL_PD_OFF 1
#define PLL_PD_ON 0

#define SYS_CRG_BASE 0x13020000u
#define AON_CRG_BASE 0x17000000u

#define CLK_CPU_ROOT_OFFSET 0x0
#define CLK_CPU_ROOT_SW_SHIFT 24
#define CLK_CPU_ROOT_SW_MASK 0x1000000u

#define CLK_PERH_ROOT_OFFSET 0x10
#define CLK_PERH_ROOT_SHIFT 24
#define CLK_PERH_ROOT_MASK 0x1000000u

#define CLK_BUS_ROOT_OFFSET 0x14
#define CLK_BUS_ROOT_SW_SHIFT 24
#define CLK_BUS_ROOT_SW_MASK 0x1000000u

#define CLK_NOC_BUS_STG_AXI_OFFSET 0x180
#define CLK_NOC_BUS_STG_AXI_EN_SHIFT 31
#define CLK_NOC_BUS_STG_AXI_EN_MASK 0x80000000u

#define CLK_AON_APB_FUNC_OFFSET 0x4
#define CLK_AON_APB_FUNC_SW_SHIFT 24
#define CLK_AON_APB_FUNC_SW_MASK 0x1000000u

#define CLK_QSPI_REF_OFFSET 0x168
#define CLK_QSPI_REF_SW_SHIFT 24
#define CLK_QSPI_REF_SW_MASK 0x1000000u

// ---- PLL1 fields (from U-Boot jh7110 pll.c) ----
#define PLL1_DACPD_MASK BIT(15)
#define PLL1_DSMPD_MASK BIT(16)
#define PLL1_FBDIV_MASK GENMASK(28, 17)
#define PLL1_PD_MASK BIT(27)
#define PLL1_POSTDIV1_MASK GENMASK(29, 28)
#define PLL1_PREDIV_MASK GENMASK(5, 0)

#define PLL1_DACPD_OFFSET 0x24
#define PLL1_DSMPD_OFFSET 0x24
#define PLL1_FBDIV_OFFSET 0x24
#define PLL1_PD_OFFSET 0x28
#define PLL1_POSTDIV1_OFFSET 0x28
#define PLL1_PREDIV_OFFSET 0x2c

// ---- DDR bus clock select (from U-Boot starfive_ddr.h) ----
#define JH7110_SYS_CRG 0x13020000u
#define DDR_BUS_MASK GENMASK(29, 24)
#define DDR_BUS_OFFSET 0xAC

#define DDR_BUS_OSC_DIV2 0
#define DDR_BUS_PLL1_DIV2 1

#define DDR_REG_SET_BUS(val)                                                   \
  clrsetbits32(JH7110_SYS_CRG + DDR_BUS_OFFSET, DDR_BUS_MASK,                  \
               ((val) << __ffs(DDR_BUS_MASK)) & DDR_BUS_MASK)

static inline u32 __ffs(u32 x) {
  u32 r = 0;
  while ((x & 1u) == 0u) {
    x >>= 1;
    r++;
  }
  return r;
}

static inline u32 mmio_read32(uptr addr) { return *(volatile u32 *)addr; }

static inline void mmio_write32(uptr addr, u32 val) {
  *(volatile u32 *)addr = val;
}

static inline void clrsetbits32(uptr addr, u32 mask, u32 val) {
  mmio_write32(addr, (mmio_read32(addr) & ~mask) | (val & mask));
}

#define SET_PLL(offset, mask, val)                                             \
  clrsetbits32(SYS_SYSCON_BASE + (offset), (mask),                             \
               ((val) << __ffs(mask)) & (mask))

static inline void udelay(u32 usec) {
  volatile u32 count = usec * 1000u;
  while (count--) {
    asm volatile("nop");
  }
}

// From u-boot jh7110_pll0_freq[] and jh7110_pll2_freq[]
static void pll0_set_1ghz(void) {
  // prediv=3, fbdiv=125, postdiv1=1, dacpd=1, dsmpd=1
  SET_PLL(PLL0_PD_OFFSET, PLL0_PD_MASK, PLL_PD_OFF);
  SET_PLL(PLL0_DACPD_OFFSET, PLL0_DACPD_MASK, 1);
  SET_PLL(PLL0_DSMPD_OFFSET, PLL0_DSMPD_MASK, 1);
  SET_PLL(PLL0_PREDIV_OFFSET, PLL0_PREDIV_MASK, 3);
  SET_PLL(PLL0_FBDIV_OFFSET, PLL0_FBDIV_MASK, 125);
  SET_PLL(PLL0_POSTDIV1_OFFSET, PLL0_POSTDIV1_MASK, (1u >> 1));
  SET_PLL(PLL0_PD_OFFSET, PLL0_PD_MASK, PLL_PD_ON);
}

static void pll2_set_1188mhz(void) {
  // prediv=2, fbdiv=99, postdiv1=1, dacpd=1, dsmpd=1
  SET_PLL(PLL2_PD_OFFSET, PLL2_PD_MASK, PLL_PD_OFF);
  SET_PLL(PLL2_DACPD_OFFSET, PLL2_DACPD_MASK, 1);
  SET_PLL(PLL2_DSMPD_OFFSET, PLL2_DSMPD_MASK, 1);
  SET_PLL(PLL2_PREDIV_OFFSET, PLL2_PREDIV_MASK, 2);
  SET_PLL(PLL2_FBDIV_OFFSET, PLL2_FBDIV_MASK, 99);
  SET_PLL(PLL2_POSTDIV1_OFFSET, PLL2_POSTDIV1_MASK, (1u >> 1));
  SET_PLL(PLL2_PD_OFFSET, PLL2_PD_MASK, PLL_PD_ON);
}

static void pll1_set_1400mhz(void) {
  // prediv=6, fbdiv=350, postdiv1=1, dacpd=1, dsmpd=1
  SET_PLL(PLL1_PD_OFFSET, PLL1_PD_MASK, PLL_PD_OFF);
  SET_PLL(PLL1_DACPD_OFFSET, PLL1_DACPD_MASK, 1);
  SET_PLL(PLL1_DSMPD_OFFSET, PLL1_DSMPD_MASK, 1);
  SET_PLL(PLL1_PREDIV_OFFSET, PLL1_PREDIV_MASK, 6);
  SET_PLL(PLL1_FBDIV_OFFSET, PLL1_FBDIV_MASK, 350);
  SET_PLL(PLL1_POSTDIV1_OFFSET, PLL1_POSTDIV1_MASK, (1u >> 1));
  SET_PLL(PLL1_PD_OFFSET, PLL1_PD_MASK, PLL_PD_ON);
}

#endif
