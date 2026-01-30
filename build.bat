set "BUILD_DIR=./build"
set "OUT_DIR=./out"
set "KERNEL_DIR=./kernel"
set "BOARD_DIR=./board"
set "BOOT_DIR=./boot"
set "COMMON_DIR=./common"

if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"

if exist "%OUT_DIR%" (
    rmdir /s /q "%OUT_DIR%"
)
mkdir "%OUT_DIR%"

:: -Os              = optimize for size (important for SRAM-limited SPL)
:: -ffreestanding   = tell compiler there is NO OS or standard runtime
:: -fno-builtin     = prevent GCC from inserting calls to libc functions (memcpy, memset, etc.)
:: -fno-pic         = disable position-independent code (no GOT / relocations)
:: -nostdlib        = do not link against libc or libgcc
:: -nostartfiles    = do not use default C runtime startup (crt0)
:: -march / -mabi   = same ISA and ABI as assembly code (must match!)

:: COMMON

riscv-none-elf-gcc -c %COMMON_DIR%/mem.c -o %BUILD_DIR%/mem.o ^
  -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
  -fno-tree-loop-distribute-patterns ^
  -nostdlib -nostartfiles -march=rv64imac_zicsr -mabi=lp64

:: BOARD

riscv-none-elf-gcc -c %BOARD_DIR%/uart0.c -o %BUILD_DIR%/uart0.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOARD_DIR%/ack_led.c -o %BUILD_DIR%/ack_led.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOARD_DIR%/board_init.c -o %BUILD_DIR%/board_init.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOARD_DIR%/ddr_init.c -o %BUILD_DIR%/ddr_init.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOARD_DIR%/ddrphy_start.c -o %BUILD_DIR%/ddrphy_start.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOARD_DIR%/ddrphy_train.c -o %BUILD_DIR%/ddrphy_train.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOARD_DIR%/ddrphy_utils.c -o %BUILD_DIR%/ddrphy_utils.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOARD_DIR%/ddrcsr_boot.c -o %BUILD_DIR%/ddrcsr_boot.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOARD_DIR%/qspi.c -o %BUILD_DIR%/qspi.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

:: BOOT
riscv-none-elf-gcc -c %BOOT_DIR%/boot.S -o %BUILD_DIR%/boot.o ^
    -march=rv64imac_zicsr_zifencei -mabi=lp64

riscv-none-elf-gcc -c %BOOT_DIR%/boot_start.c -o %BUILD_DIR%/boot_start.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64 ^
    -DDEBUG=1

riscv-none-elf-gcc -o %BUILD_DIR%/boot.elf ^
    %BUILD_DIR%/mem.o ^
    %BUILD_DIR%/board_init.o ^
    %BUILD_DIR%/ddr_init.o ^
    %BUILD_DIR%/ddrphy_start.o ^
    %BUILD_DIR%/ddrphy_train.o ^
    %BUILD_DIR%/ddrphy_utils.o ^
    %BUILD_DIR%/ddrcsr_boot.o ^
    %BUILD_DIR%/qspi.o ^
    %BUILD_DIR%/uart0.o ^
    %BUILD_DIR%/ack_led.o ^
    %BUILD_DIR%/boot.o ^
    %BUILD_DIR%/boot_start.o ^
    -nostdlib -nostartfiles ^
    -Wl,-m,elf64lriscv -Wl,-T,%BOOT_DIR%/boot.ld -Wl,--gc-sections ^
    -Wl,--no-warn-rwx-segments

:: Convert ELF file to raw binary
:: -O binary = strip ELF headers and metadata
:: Result is a flat image suitable for ROM / SD / SPI loading

:: Get end address (hex) from boot.elf
for /f "tokens=1,3" %%A in ('riscv-none-elf-nm -n %BUILD_DIR%/boot.elf ^| findstr /r "\<boot_end\>"') do set "BOOT_END_HEX=%%A"

echo boot_end = %BOOT_END_HEX%

:: Produce boot.bin padded up to end
riscv-none-elf-objcopy -O binary ^
  --gap-fill 0x00 ^
  --pad-to 0x%BOOT_END_HEX% ^
  %BUILD_DIR%/boot.elf %OUT_DIR%/boot.bin

:: SUPERVISOR
riscv-none-elf-gcc -c %BOOT_DIR%/supervisor.S -o %BUILD_DIR%/supervisor.o ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %BOOT_DIR%/supervisor_start.c -o %BUILD_DIR%/supervisor_start.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64 ^
    -DDEBUG=1

riscv-none-elf-gcc -o %BUILD_DIR%/supervisor.elf ^
    %BUILD_DIR%/mem.o ^
    %BUILD_DIR%/ack_led.o ^
    %BUILD_DIR%/supervisor.o ^
    %BUILD_DIR%/supervisor_start.o ^
    -nostdlib -nostartfiles ^
    -Wl,-m,elf64lriscv -Wl,-T,%BOOT_DIR%/supervisor.ld -Wl,--gc-sections ^
    -Wl,--no-warn-rwx-segments

:: Convert ELF file to raw binary
:: -O binary = strip ELF headers and metadata
:: Result is a flat image suitable for ROM / SD / SPI loading

:: Get end address (hex) from boot.elf
for /f "tokens=1,3" %%A in ('riscv-none-elf-nm -n %BUILD_DIR%/supervisor.elf ^| findstr /r "\<supervisor_end\>"') do set "SUPERVISOR_END_HEX=%%A"

echo supervisor_end = %SUPERVISOR_END_HEX%

:: Produce boot.bin padded up to end
riscv-none-elf-objcopy -O binary ^
  --gap-fill 0x00 ^
  --pad-to 0x%SUPERVISOR_END_HEX% ^
  %BUILD_DIR%/supervisor.elf %OUT_DIR%/supervisor.bin

for %%F in ("%OUT_DIR%\supervisor.bin") do set "SUPERVISOR_SIZE=%%~zF"
echo supervisor size = %SUPERVISOR_SIZE% bytes
set /a KERNEL_ADDRESS_OFFSET=SUPERVISOR_SIZE + 0x40000000
echo kernel address offset = %KERNEL_ADDRESS_OFFSET%

:: KERNEL

:: march = Target RISC-V ISA (which instructions the CPU supports)
:: rv64  = 64-bit RISC-V
:: i     = base integer instruction set (mandatory)
:: m     = multiply/divide instructions
:: a     = atomic instructions
:: c     = compressed 16-bit instructions (smaller code size)
::
:: mabi  = ABI (how functions pass arguments, use registers, and lay out the stack)
:: lp64  = 64-bit longs and pointers, integer-only ABI (no FPU usage)

riscv-none-elf-gcc -c %KERNEL_DIR%/entry.S -o %BUILD_DIR%/entry.o ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %KERNEL_DIR%/trampoline.S -o %BUILD_DIR%/trampoline.o ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %KERNEL_DIR%/trap.S -o %BUILD_DIR%/trap.o ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %KERNEL_DIR%/mtrap.S -o %BUILD_DIR%/mtrap.o ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %KERNEL_DIR%/start.c -o %BUILD_DIR%/start.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %KERNEL_DIR%/main.c -o %BUILD_DIR%/main.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %KERNEL_DIR%/proc.c -o %BUILD_DIR%/proc.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

riscv-none-elf-gcc -c %KERNEL_DIR%/spinlock.c -o %BUILD_DIR%/spinlock.o ^
    -Os -ffreestanding -fno-builtin -fno-pic -msmall-data-limit=0 ^
    -nostdlib -nostartfiles ^
    -march=rv64imac_zicsr -mabi=lp64

:: Link stage:
:: -nostdlib        = do not pull in libc or libgcc
:: -nostartfiles    = do not add default startup objects
:: -Wl,-T,linker.ld = use custom linker script (controls load address, memory layout)
:: -Wl,--gc-sections= garbage-collect unused code/data sections (reduces SPL size)

riscv-none-elf-gcc -o %BUILD_DIR%/kernel.elf ^
    %BUILD_DIR%/mem.o ^
    %BUILD_DIR%/uart0.o ^
    %BUILD_DIR%/entry.o ^
    %BUILD_DIR%/trampoline.o ^
    %BUILD_DIR%/trap.o ^
    %BUILD_DIR%/mtrap.o ^
    %BUILD_DIR%/start.o ^
    %BUILD_DIR%/main.o ^
    %BUILD_DIR%/proc.o ^
    %BUILD_DIR%/spinlock.o ^
    -nostdlib -nostartfiles ^
    -Wl,--defsym,KERNEL_START=%KERNEL_ADDRESS_OFFSET% ^
    -Wl,-m,elf64lriscv -Wl,-T,%KERNEL_DIR%/kernel.ld -Wl,--gc-sections ^
    -Wl,--no-warn-rwx-segments

:: Convert ELF file to raw binary
:: -O binary = strip ELF headers and metadata
:: Result is a flat image suitable for ROM / SD / SPI loading

:: Get end address (hex) from kernel.elf
for /f "tokens=1,3" %%A in ('riscv-none-elf-nm -n %BUILD_DIR%/kernel.elf ^| findstr /r "\<kernel_end\>"') do set "KERNEL_END_HEX=%%A"

echo kernel_end = %KERNEL_END_HEX%

:: Produce kernel.bin padded up to end
riscv-none-elf-objcopy -O binary ^
  --gap-fill 0x00 ^
  --pad-to 0x%KERNEL_END_HEX% ^
  %BUILD_DIR%/kernel.elf %OUT_DIR%/kernel.bin

:: GLUE

:: Write 32-byte header: 
::  u64 magic "SUPERVISORHDR1" + u64 supervisor_size (little-endian)
::  u64 magic "KRNLHDR1" + u64 kernel_size (little-endian)
powershell -NoProfile -Command ^
  "$sb = Get-Item '%OUT_DIR%\supervisor.bin';" ^
  "$smagic = [UInt64]0x6D8CF37B26F312B4;" ^
  "$ssize = [UInt64]$sb.Length;" ^
  "$kb = Get-Item '%OUT_DIR%\kernel.bin';" ^
  "$kmagic = [UInt64]0x4B524E4C48445231;" ^
  "$ksize = [UInt64]$kb.Length;" ^
  "$bytes = New-Object byte[] 32;" ^
  "[BitConverter]::GetBytes([UInt64]$smagic).CopyTo($bytes,0);" ^
  "[BitConverter]::GetBytes([UInt64]$ssize).CopyTo($bytes,8);" ^
  "[BitConverter]::GetBytes([UInt64]$kmagic).CopyTo($bytes,16);" ^
  "[BitConverter]::GetBytes([UInt64]$ksize).CopyTo($bytes,24);" ^
  "[IO.File]::WriteAllBytes('%OUT_DIR%/binary.hdr',$bytes)"

copy /b "%OUT_DIR%\boot.bin"+"%OUT_DIR%\binary.hdr"+"%OUT_DIR%\supervisor.bin"+"%OUT_DIR%\kernel.bin" "%OUT_DIR%\gameos.bin"

:: Use starfire tool to create the out file
wsl.exe -- bash -lc "cd ../Tools/spl_tool && ./spl_tool -c -f ../../gameos/%OUT_DIR%/gameos.bin"
