# Third-party code

This project contains code from the projects below. Their copyright notices are kept in the source files where they existed, and their license texts are in [`LICENSES/`](LICENSES/).

## StarFive DDR initialization (from U-Boot) — GPL-2.0-or-later

Copyright (C) 2022-2023 StarFive Technology Co., Ltd.

The LPDDR4 controller and PHY bring-up sequence comes from StarFive's DDR driver in U-Boot:

- `board/ddrcsr_boot.c`
- `board/ddrphy_start.c`
- `board/ddrphy_train.c`
- `board/ddrphy_utils.c`
- `board/starfive_ddr.h`

Some register offsets, clock/PLL values and the QSPI controller setup in `board/jh7110.h` and `board/qspi.c` are taken from U-Boot (`pll.c`, `starfive_ddr.h`, `cadence_qspi_apb.c`) and the Linux JH7110 device-tree bindings, as noted in comments in those files.

License: [GPL-2.0](LICENSES/GPL-2.0.txt)

## xv6 (RISC-V) — MIT

Copyright (c) 2006-2024 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology

The kernel in `kernel/` is a port of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) in progress. The machine-mode setup (`start.c`), CSR helpers (`riscv.h`), trampoline (`trampoline.S`), spinlocks and per-CPU structures follow xv6, adapted for the JH7110S.

License: [MIT](LICENSES/xv6-MIT.txt)

## Packaging tool

The final image is packaged with StarFive's [`spl_tool`](https://github.com/starfive-tech/Tools). It is not included in this repository.
