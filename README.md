# GameOS for the VisionFive 2 Lite

A bare-metal operating system for the [StarFive VisionFive 2 Lite](https://www.waveshare.com/visionfive2-lite.htm) RISC-V board, written from the first instruction the CPU runs after the boot ROM.

The long-term goal is a custom handheld PC: this board, a custom OS, and games written specifically for it. The project is a **work in progress**. Right now it boots from QSPI flash, brings up DDR memory, loads itself into DDR, and starts an xv6-based kernel on the application cores.

## Hardware

| | |
|---|---|
| Board | StarFive VisionFive 2 Lite |
| SoC | StarFive JH7110S |
| Cores | 1 × SiFive S7 monitor core (hart 0, no MMU) + 4 × SiFive U74 application cores (harts 1–4), RV64 |
| Memory | LPDDR4 |
| Boot media | QSPI NOR flash |
| Console | UART0 (16550-compatible), 115200 baud |

## Boot flow

```
 BootROM
   │  loads the SPL image from QSPI flash into on-chip SRAM
   ▼
 boot/boot.S  ── all harts start here
   │  hart 0 (S7) ───────────────────────────────┐   harts 1–4 (U74)
   ▼                                             │   spin until a kernel
 boot_start()  (boot/boot_start.c)               │   address is published
   ├─ LED + UART0 init                           │
   ├─ clocks / PLLs / board init                 │
   ├─ LPDDR4 controller + PHY init and training  │
   ├─ DDR sanity test                            │
   ├─ QSPI init (memory-mapped reads)            │
   ├─ read image header, check magic numbers     │
   ├─ copy supervisor + kernel into DDR, verify  │
   └─ publish start addresses ──────────────────►│
   ▼                                             ▼
 supervisor (hart 0)                        kernel (harts 1–4)
 boot/supervisor_start.c                    kernel/entry.S → start.c
 LED heartbeat for now                      M-mode setup: trap delegation, PMP,
                                            CLINT timer → mret into S-mode
                                            → main()
```

### Image layout in flash

```
┌──────────────┬─────────────────────────────┬────────────────┬────────────┐
│ boot.bin     │ header (32 bytes)           │ supervisor.bin │ kernel.bin │
│ (SPL, SRAM)  │ u64 supervisor magic        │                │            │
│              │ u64 supervisor size         │                │            │
│              │ u64 kernel magic "KRNLHDR1" │                │            │
│              │ u64 kernel size             │                │            │
└──────────────┴─────────────────────────────┴────────────────┴────────────┘
```

`build.bat` builds the three parts, writes the header, concatenates them into `out/gameos.bin`, and runs `spl_tool` to add the SPL header the boot ROM expects.

## Status

**Working on hardware:**
- Boot loader from the boot ROM: clocks, UART console, LED, QSPI flash
- LPDDR4 bring-up and a DDR read/write test
- Loading the supervisor and kernel from flash into DDR, with copy verification
- Releasing the U74 cores into the kernel

**In progress:**
- Kernel bring-up on the U74 cores: machine-mode setup, trap delegation, timer interrupts and the switch to supervisor mode (written, being debugged)
- Porting the rest of xv6: paging, processes, scheduler, system calls, file system

**Planned:**
- Display output
- Input from the handheld's buttons
- A small runtime and the games themselves

## Repository layout

| Folder | Contents |
|---|---|
| `boot/` | Entry point for all harts, the boot loader (`boot_start.c`) and the supervisor stage, with linker scripts |
| `board/` | JH7110S drivers: clocks and PLLs, UART0, LED, QSPI, DDR init |
| `kernel/` | xv6-based kernel: entry, M-mode setup, traps, trampoline, spinlocks, per-CPU state |
| `common/` | Freestanding `memcpy` / `memset` / `memmove` and shared types |

## Building

Requirements:
- Windows with the [xPack RISC-V GCC toolchain](https://xpack-dev-tools.github.io/riscv-none-elf-gcc-xpack/) (`riscv-none-elf-gcc`) on `PATH`
- WSL with StarFive's [`spl_tool`](https://github.com/starfive-tech/Tools) built inside it

```bat
:: Optional: where spl_tool is (default: ..\Tools\spl_tool)
set SPL_TOOL_DIR=C:\path\to\Tools\spl_tool
build.bat
```

The output is `out/gameos.bin.normal.out`, an image with the SPL header that the boot ROM can load.

## Flashing and running

1. Connect a USB-to-serial adapter to UART0 (115200 baud).
2. Put the board in **UART boot mode** and power it on. The boot ROM waits for an XMODEM transfer.
3. Send StarFive's recovery/flashing tool ([`jh7110-recovery-*.bin`](https://github.com/starfive-tech/Tools/tree/master/recovery)) over XMODEM.
4. In the tool's menu, choose **update SPL** and send `out/gameos.bin.normal.out`. It is written to the SPL area of the QSPI flash.
5. Switch back to **QSPI boot mode** and reset. The boot log appears on UART0.

See StarFive's [Recovering the Bootloader](https://doc-en.rvspace.org/VisionFive2/Quick_Start_Guide/VisionFive2_SDK_QSG/recovering_bootloader%20-%20vf2.html) guide for details.

## License

This project is licensed under the **GNU General Public License v2.0 or later**. See [LICENSE](LICENSE).

It includes StarFive's DDR initialization code from U-Boot (GPL-2.0-or-later) and code derived from MIT's xv6 (MIT). See [THIRD_PARTY.md](THIRD_PARTY.md) for details.
