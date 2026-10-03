#include "jh7110.h"
#include "qspi.h"
#include "uart0.h"

// Cadence QSPI controller base (reg) and AHB window are fixed on JH7110.
#define QSPI_REG_BASE 0x13010000u

// Minimal register/bit set (from U-Boot cadence_qspi_apb.c)
#define CQSPI_REG_CONFIG 0x00
#define CQSPI_REG_CONFIG_ENABLE BIT(0)
#define CQSPI_REG_CONFIG_DIRECT BIT(7)
#define CQSPI_REG_CONFIG_DECODE BIT(9)
#define CQSPI_REG_CONFIG_CHIPSELECT_LSB 10
#define CQSPI_REG_CONFIG_CHIPSELECT_MASK 0xF
#define CQSPI_REG_CONFIG_BAUD_LSB 19
#define CQSPI_REG_CONFIG_BAUD_MASK 0xF

#define CQSPI_REG_RD_INSTR 0x04
#define CQSPI_REG_RD_INSTR_OPCODE_LSB 0
#define CQSPI_REG_RD_INSTR_TYPE_INSTR_LSB 8
#define CQSPI_REG_RD_INSTR_TYPE_ADDR_LSB 12
#define CQSPI_REG_RD_INSTR_TYPE_DATA_LSB 16

#define CQSPI_REG_DELAY 0x0C
#define CQSPI_REG_DELAY_TSLCH_LSB 0
#define CQSPI_REG_DELAY_TCHSH_LSB 8
#define CQSPI_REG_DELAY_TSD2D_LSB 16
#define CQSPI_REG_DELAY_TSHSL_LSB 24
#define CQSPI_REG_DELAY_TSLCH_MASK 0xFF
#define CQSPI_REG_DELAY_TCHSH_MASK 0xFF
#define CQSPI_REG_DELAY_TSD2D_MASK 0xFF
#define CQSPI_REG_DELAY_TSHSL_MASK 0xFF

#define CQSPI_REG_RD_DATA_CAPTURE 0x10
#define CQSPI_REG_RD_DATA_CAPTURE_BYPASS BIT(0)

#define CQSPI_REG_SIZE 0x14
#define CQSPI_REG_SIZE_ADDRESS_LSB 0
#define CQSPI_REG_SIZE_ADDRESS_MASK 0xF
#define CQSPI_REG_SIZE_PAGE_LSB 4
#define CQSPI_REG_SIZE_PAGE_MASK 0xFFF
#define CQSPI_REG_SIZE_BLOCK_LSB 16
#define CQSPI_REG_SIZE_BLOCK_MASK 0x3F

#define CQSPI_REG_REMAP 0x24
#define CQSPI_REG_IRQMASK 0x44

#define CQSPI_REG_CMDCTRL 0x90
#define CQSPI_REG_CMDCTRL_EXECUTE BIT(0)
#define CQSPI_REG_CMDCTRL_INPROGRESS BIT(1)
#define CQSPI_REG_CMDCTRL_RD_BYTES_LSB 20
#define CQSPI_REG_CMDCTRL_RD_BYTES_MASK 0x7
#define CQSPI_REG_CMDCTRL_RD_EN_LSB 23
#define CQSPI_REG_CMDCTRL_OPCODE_LSB 24

#define CQSPI_REG_CMDREADDATALOWER 0xA0

#define CQSPI_REG_SDRAMLEVEL 0x2C
#define CQSPI_REG_SDRAMLEVEL_RD_LSB 0
#define CQSPI_REG_SDRAMLEVEL_RD_MASK 0xFFFF

#define CQSPI_REG_SRAMPARTITION 0x18
#define CQSPI_REG_INDIRECTTRIGGER 0x1C
#define CQSPI_REG_INDIRECTRD 0x60
#define CQSPI_REG_INDIRECTRD_START BIT(0)
#define CQSPI_REG_INDIRECTRD_DONE BIT(5)
#define CQSPI_REG_INDIRECTRDSTARTADDR 0x68
#define CQSPI_REG_INDIRECTRDBYTES 0x6C

// Cadence instruction types.
#define CQSPI_INST_TYPE_SINGLE 0

// SYSCRG clock gate register offsets (from dt-bindings/clock/starfive,jh7110-crg.h)
#define QSPI_CLK_AHB_OFFSET (87u * 4u)
#define QSPI_CLK_APB_OFFSET (88u * 4u)
#define QSPI_REF_SRC_OFFSET (89u * 4u)
#define QSPI_CLK_REF_OFFSET (90u * 4u)
#define CLK_ENABLE_MASK 0x80000000u

// JH7110 reset IDs for QSPI (from dt-bindings/reset/starfive-jh7110.h)
#define RSTN_U0_CDNS_QSPI_AHB 61
#define RSTN_U0_CDNS_QSPI_APB 62
#define RSTN_U0_CDNS_QSPI_REF 63

static inline u32 qspi_read32(u32 offset) {
  return mmio_read32(QSPI_REG_BASE + offset);
}

static inline void qspi_write32(u32 offset, u32 val) {
  mmio_write32(QSPI_REG_BASE + offset, val);
}

static inline void qspi_set_bits(uptr addr, u32 mask) {
  mmio_write32(addr, mmio_read32(addr) | mask);
}

static inline void qspi_disable(void) {
  u32 reg = qspi_read32(CQSPI_REG_CONFIG);
  reg &= ~CQSPI_REG_CONFIG_ENABLE;
  qspi_write32(CQSPI_REG_CONFIG, reg);
}

static inline void qspi_enable(void) {
  u32 reg = qspi_read32(CQSPI_REG_CONFIG);
  reg |= CQSPI_REG_CONFIG_ENABLE;
  qspi_write32(CQSPI_REG_CONFIG, reg);
}

static inline void qspi_reset_deassert(u32 reset_id) {
  u32 mask = BIT(reset_id & 31u);
  u32 reg = mmio_read32(SYS_CRG_BASE + SYSCRG_RESET_ASSERT1_OFFSET);
  reg &= ~mask;
  mmio_write32(SYS_CRG_BASE + SYSCRG_RESET_ASSERT1_OFFSET, reg);
}

static inline void qspi_set_baud_div(u32 ref_clk_hz, u32 sclk_hz) {
  u32 div;
  u32 reg;

  // div = ceil(ref / (2*sclk)) - 1, clamp to 4-bit.
  div = (ref_clk_hz + (sclk_hz * 2u - 1u)) / (sclk_hz * 2u);
  if (div > 0)
    div -= 1;
  if (div > CQSPI_REG_CONFIG_BAUD_MASK)
    div = CQSPI_REG_CONFIG_BAUD_MASK;

  qspi_disable();
  reg = qspi_read32(CQSPI_REG_CONFIG);
  reg &= ~(CQSPI_REG_CONFIG_BAUD_MASK << CQSPI_REG_CONFIG_BAUD_LSB);
  reg |= (div & CQSPI_REG_CONFIG_BAUD_MASK) << CQSPI_REG_CONFIG_BAUD_LSB;
  qspi_write32(CQSPI_REG_CONFIG, reg);
  qspi_enable();
}

static inline void qspi_set_delays(u32 ref_clk_hz, u32 sclk_hz,
                                   u32 tshsl_ns, u32 tsd2d_ns,
                                   u32 tchsh_ns, u32 tslch_ns) {
  // Same math as U-Boot, reduced.
  u32 ref_clk_ns = (1000000000u + ref_clk_hz - 1u) / ref_clk_hz;
  u32 sclk_ns = (1000000000u + sclk_hz - 1u) / sclk_hz;
  u32 tshsl, tsd2d, tchsh, tslch;
  u32 reg;

  if (tshsl_ns >= sclk_ns + ref_clk_ns)
    tshsl_ns -= sclk_ns + ref_clk_ns;
  if (tchsh_ns >= sclk_ns + 3u * ref_clk_ns)
    tchsh_ns -= sclk_ns + 3u * ref_clk_ns;

  tshsl = (tshsl_ns + ref_clk_ns - 1u) / ref_clk_ns;
  tsd2d = (tsd2d_ns + ref_clk_ns - 1u) / ref_clk_ns;
  tchsh = (tchsh_ns + ref_clk_ns - 1u) / ref_clk_ns;
  tslch = (tslch_ns + ref_clk_ns - 1u) / ref_clk_ns;

  qspi_disable();
  reg = ((tshsl & CQSPI_REG_DELAY_TSHSL_MASK) << CQSPI_REG_DELAY_TSHSL_LSB) |
        ((tchsh & CQSPI_REG_DELAY_TCHSH_MASK) << CQSPI_REG_DELAY_TCHSH_LSB) |
        ((tslch & CQSPI_REG_DELAY_TSLCH_MASK) << CQSPI_REG_DELAY_TSLCH_LSB) |
        ((tsd2d & CQSPI_REG_DELAY_TSD2D_MASK) << CQSPI_REG_DELAY_TSD2D_LSB);
  qspi_write32(CQSPI_REG_DELAY, reg);
  qspi_enable();
}

static inline void qspi_select_cs0(void) {
  u32 reg = qspi_read32(CQSPI_REG_CONFIG);
  reg &= ~CQSPI_REG_CONFIG_DECODE;
  reg &= ~(CQSPI_REG_CONFIG_CHIPSELECT_MASK << CQSPI_REG_CONFIG_CHIPSELECT_LSB);
  reg |= (0xEu & CQSPI_REG_CONFIG_CHIPSELECT_MASK)
         << CQSPI_REG_CONFIG_CHIPSELECT_LSB;
  qspi_write32(CQSPI_REG_CONFIG, reg);
}

static void qspi_put_hex32(u32 v) {
  static const char hex[] = "0123456789ABCDEF";
  for (int i = 7; i >= 0; i--) {
    uart0_putc(hex[(v >> (i * 4)) & 0xFu]);
  }
}

static u32 qspi_read_jedec_id(void) {
  u32 reg = (0x9Fu << CQSPI_REG_CMDCTRL_OPCODE_LSB) |
            (1u << CQSPI_REG_CMDCTRL_RD_EN_LSB) |
            ((3u - 1u) & CQSPI_REG_CMDCTRL_RD_BYTES_MASK)
                << CQSPI_REG_CMDCTRL_RD_BYTES_LSB;
  qspi_write32(CQSPI_REG_CMDCTRL, reg);
  qspi_write32(CQSPI_REG_CMDCTRL, reg | CQSPI_REG_CMDCTRL_EXECUTE);

  for (u32 i = 0; i < 10000u; i++) {
    if ((qspi_read32(CQSPI_REG_CMDCTRL) & CQSPI_REG_CMDCTRL_INPROGRESS) == 0u) {
      return qspi_read32(CQSPI_REG_CMDREADDATALOWER) & 0x00FFFFFFu;
    }
    udelay(1);
  }

  return 0;
}

void qspi_indirect_read(u32 addr, u8 *dst, u32 len) {
  if (len == 0) {
    return;
  }

  // Trigger address 0, half SRAM for RX.
  qspi_write32(CQSPI_REG_INDIRECTTRIGGER, 0);
  qspi_write32(CQSPI_REG_SRAMPARTITION, 128);

  qspi_write32(CQSPI_REG_INDIRECTRDSTARTADDR, addr);
  qspi_write32(CQSPI_REG_INDIRECTRDBYTES, len);
  qspi_write32(CQSPI_REG_INDIRECTRD, CQSPI_REG_INDIRECTRD_START);

  u32 remaining = len;
  u8 *p = dst;
  u32 timeout = 1000000u;
  volatile u8 *ahb = qspi_ahb_base();
  while (remaining && timeout--) {
    u32 level = (qspi_read32(CQSPI_REG_SDRAMLEVEL) >>
                 CQSPI_REG_SDRAMLEVEL_RD_LSB) &
                CQSPI_REG_SDRAMLEVEL_RD_MASK;
    if (level == 0) {
      continue;
    }

    u32 bytes = level * 4u;
    if (bytes > remaining) {
      bytes = remaining;
    }

    for (u32 i = 0; i < bytes; i++) {
      p[i] = ahb[i];
    }
    p += bytes;
    remaining -= bytes;
  }

  // Timeout: fill remaining bytes with zero.
  while (remaining) {
    *p++ = 0;
    remaining--;
  }

  // Clear done flag if set.
  if (qspi_read32(CQSPI_REG_INDIRECTRD) & CQSPI_REG_INDIRECTRD_DONE) {
    qspi_write32(CQSPI_REG_INDIRECTRD, CQSPI_REG_INDIRECTRD_DONE);
  }
}

void qspi_init(void) {
  // Enable QSPI clocks (AHB/APB/REF). Keep ref source on pll2 via boardinit.
  qspi_set_bits(SYS_CRG_BASE + QSPI_CLK_AHB_OFFSET, CLK_ENABLE_MASK);
  qspi_set_bits(SYS_CRG_BASE + QSPI_CLK_APB_OFFSET, CLK_ENABLE_MASK);
  qspi_set_bits(SYS_CRG_BASE + QSPI_REF_SRC_OFFSET, CLK_ENABLE_MASK);
  qspi_set_bits(SYS_CRG_BASE + QSPI_CLK_REF_OFFSET, CLK_ENABLE_MASK);

  // Select QSPI ref source = osc (24MHz) for safe early-boot timing.
  clrsetbits32(SYS_CRG_BASE + CLK_QSPI_REF_OFFSET, CLK_QSPI_REF_SW_MASK, 0);

  // Deassert QSPI resets (APB/AHB/REF) in SYSCRG reset group1.
  qspi_reset_deassert(RSTN_U0_CDNS_QSPI_AHB);
  qspi_reset_deassert(RSTN_U0_CDNS_QSPI_APB);
  qspi_reset_deassert(RSTN_U0_CDNS_QSPI_REF);

  // Basic controller init: set sizes, delays, and read opcode.
  qspi_disable();

  // Size config: 3-byte addresses, 256-byte page, 16-byte block (U-Boot defaults).
  {
    u32 reg = qspi_read32(CQSPI_REG_SIZE);
    reg &= ~CQSPI_REG_SIZE_ADDRESS_MASK;
    reg |= (3u - 1u) << CQSPI_REG_SIZE_ADDRESS_LSB;
    reg &= ~(CQSPI_REG_SIZE_PAGE_MASK << CQSPI_REG_SIZE_PAGE_LSB);
    reg |= (256u & CQSPI_REG_SIZE_PAGE_MASK) << CQSPI_REG_SIZE_PAGE_LSB;
    reg &= ~(CQSPI_REG_SIZE_BLOCK_MASK << CQSPI_REG_SIZE_BLOCK_LSB);
    reg |= (16u & CQSPI_REG_SIZE_BLOCK_MASK) << CQSPI_REG_SIZE_BLOCK_LSB;
    qspi_write32(CQSPI_REG_SIZE, reg);
  }

  // No remap.
  qspi_write32(CQSPI_REG_REMAP, 0);

  // Disable interrupts.
  qspi_write32(CQSPI_REG_IRQMASK, 0);

  // Read data capture: use small delay (matches Linux DT read-delay = 2).
  qspi_write32(CQSPI_REG_RD_DATA_CAPTURE, (2u << 1));

  qspi_enable();

  // Conservative timings and baud (24MHz ref, 1MHz SCLK).
  qspi_set_baud_div(24000000u, 1000000u);
  qspi_set_delays(24000000u, 1000000u, 1u, 1u, 1u, 1u);

  // Select CS0 (non-decoded).
  qspi_select_cs0();

  // Set read instruction: opcode 0x03, 1-1-1.
  {
    u32 rd = (0x03u << CQSPI_REG_RD_INSTR_OPCODE_LSB) |
             (CQSPI_INST_TYPE_SINGLE << CQSPI_REG_RD_INSTR_TYPE_INSTR_LSB) |
             (CQSPI_INST_TYPE_SINGLE << CQSPI_REG_RD_INSTR_TYPE_ADDR_LSB) |
             (CQSPI_INST_TYPE_SINGLE << CQSPI_REG_RD_INSTR_TYPE_DATA_LSB);
    qspi_write32(CQSPI_REG_RD_INSTR, rd);
  }

  // Enable direct access mode (memory-mapped).
  {
    u32 reg = qspi_read32(CQSPI_REG_CONFIG);
    reg |= CQSPI_REG_CONFIG_DIRECT;
    qspi_write32(CQSPI_REG_CONFIG, reg);
  }

  // Debug: try to read JEDEC ID via STIG.
  {
    u32 id = qspi_read_jedec_id();
    uart0_puts("QSPI JEDEC ID: 0x");
    qspi_put_hex32(id);
    uart0_puts("\n");
  }
}
