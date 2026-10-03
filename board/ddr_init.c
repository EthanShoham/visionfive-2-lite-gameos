#include "starfive_ddr.h"

#define DDR_CTRL_BASE 0x15700000u
#define DDR_PHY_BASE  0x13000000u
#define DDR_BASE      0x40000000u

#define SYSCRG_RESET_ASSERT1 0x2FCu
#define SYSCRG_RESET_STATUS1 0x30Cu

#define RSTN_U0_DDR_AXI 38
#define RSTN_U0_DDR_OSC 39
#define RSTN_U0_DDR_APB 40

enum ddr_type_t starfive_ddr_type;

static void reset_trigger(u32 id, b32 assert) {
  u32 bit = id % 32u;
  uptr assert_reg = SYS_CRG_BASE + SYSCRG_RESET_ASSERT1;
  uptr status_reg = SYS_CRG_BASE + SYSCRG_RESET_STATUS1;
  u32 mask = 1u << bit;
  u32 v = mmio_read32(assert_reg);

  if (assert) {
    v |= mask;
  } else {
    v &= ~mask;
  }
  mmio_write32(assert_reg, v);

  for (u32 i = 0; i < 10000u; i++) {
    u32 s = mmio_read32(status_reg) & mask;
    if (assert) {
      if (s == mask) {
        break;
      }
    } else {
      if (s == 0u) {
        break;
      }
    }
  }
}

static void reset_assert(u32 id) { reset_trigger(id, 1); }
static void reset_deassert(u32 id) { reset_trigger(id, 0); }

void ddr_init_lpddr4_8g_2800(void) {
  starfive_ddr_type = DDR_TYPE_LPDDR4;

  DDR_REG_SET(BUS, DDR_BUS_OSC_DIV2);
  pll1_set_1400mhz();
  udelay(100);
  DDR_REG_SET(BUS, DDR_BUS_PLL1_DIV2);

  reset_assert(RSTN_U0_DDR_OSC);
  reset_deassert(RSTN_U0_DDR_OSC);
  reset_assert(RSTN_U0_DDR_APB);
  reset_deassert(RSTN_U0_DDR_APB);
  reset_assert(RSTN_U0_DDR_AXI);
  reset_deassert(RSTN_U0_DDR_AXI);

  ddr_phy_train((u32 *)(DDR_PHY_BASE + (PHY_BASE_ADDR << 2)));
  ddr_phy_util((u32 *)(DDR_PHY_BASE + (PHY_AC_BASE_ADDR << 2)));
  ddr_phy_start((u32 *)DDR_PHY_BASE, DDR_SIZE_8G);

  DDR_REG_SET(BUS, DDR_BUS_OSC_DIV2);
  ddrcsr_boot((u32 *)DDR_CTRL_BASE,
              (u32 *)(DDR_CTRL_BASE + SEC_CTRL_ADDR),
              (u32 *)DDR_PHY_BASE, DDR_SIZE_8G);
}
