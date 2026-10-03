#include "jh7110.h"

void ddr_init_lpddr4_8g_2800();

void boardinit() {
  pll0_set_1ghz();
  pll2_set_1188mhz();

  clrsetbits32(SYS_CRG_BASE + CLK_CPU_ROOT_OFFSET, CLK_CPU_ROOT_SW_MASK,
               (1u << CLK_CPU_ROOT_SW_SHIFT) & CLK_CPU_ROOT_SW_MASK);

  clrsetbits32(SYS_CRG_BASE + CLK_BUS_ROOT_OFFSET, CLK_BUS_ROOT_SW_MASK,
               (1u << CLK_BUS_ROOT_SW_SHIFT) & CLK_BUS_ROOT_SW_MASK);

  /* set clk_perh_root mux to pll2 */
  clrsetbits32(SYS_CRG_BASE + CLK_PERH_ROOT_OFFSET, CLK_PERH_ROOT_MASK,
               (1u << CLK_PERH_ROOT_SHIFT) & CLK_PERH_ROOT_MASK);

  /* enable NOC bus stage */
  clrsetbits32(
      SYS_CRG_BASE + CLK_NOC_BUS_STG_AXI_OFFSET, CLK_NOC_BUS_STG_AXI_EN_MASK,
      (1u << CLK_NOC_BUS_STG_AXI_EN_SHIFT) & CLK_NOC_BUS_STG_AXI_EN_MASK);

  /* set AON APB func clock source */
  clrsetbits32(AON_CRG_BASE + CLK_AON_APB_FUNC_OFFSET, CLK_AON_APB_FUNC_SW_MASK,
               (1u << CLK_AON_APB_FUNC_SW_SHIFT) & CLK_AON_APB_FUNC_SW_MASK);

  /* optional but U‑Boot does it before DDR */
  clrsetbits32(SYS_CRG_BASE + CLK_QSPI_REF_OFFSET, CLK_QSPI_REF_SW_MASK,
               (1u << CLK_QSPI_REF_SW_SHIFT) & CLK_QSPI_REF_SW_MASK);
  ddr_init_lpddr4_8g_2800();
}
