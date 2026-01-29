set "BUILD_DIR=./build"
set "OUT_DIR=./out"
set "KERNEL_DIR=./kernel"
set "BOARD_DIR=./board"

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

if not exist "%OUT_DIR%" (
    mkdir "%OUT_DIR%"
)

:: -Os              = optimize for size (important for SRAM-limited SPL)
:: -ffreestanding   = tell compiler there is NO OS or standard runtime
:: -fno-builtin     = prevent GCC from inserting calls to libc functions (memcpy, memset, etc.)
:: -fno-pic         = disable position-independent code (no GOT / relocations)
:: -nostdlib        = do not link against libc or libgcc
:: -nostartfiles    = do not use default C runtime startup (crt0)
:: -march / -mabi   = same ISA and ABI as assembly code (must match!)


:: BOARD

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

riscv-none-elf-gcc -o %BUILD_DIR%/boot.elf ^
    %BUILD_DIR%/entry.o ^
    %BUILD_DIR%/trampoline.o ^
    %BUILD_DIR%/board_init.o ^
    %BUILD_DIR%/ddr_init.o ^
    %BUILD_DIR%/ddrphy_start.o ^
    %BUILD_DIR%/ddrphy_train.o ^
    %BUILD_DIR%/ddrphy_utils.o ^
    %BUILD_DIR%/ddrcsr_boot.o ^
    %BUILD_DIR%/start.o ^
    %BUILD_DIR%/main.o ^
    %BUILD_DIR%/proc.o ^
    %BUILD_DIR%/spinlock.o ^
    -nostdlib -nostartfiles ^
    -Wl,-m,elf64lriscv -Wl,-T,%KERNEL_DIR%/kernel.ld -Wl,--gc-sections ^
    -Wl,--no-warn-rwx-segments


:: Convert ELF file to raw binary
:: -O binary = strip ELF headers and metadata
:: Result is a flat image suitable for ROM / SD / SPI loading

riscv-none-elf-objcopy -O binary %BUILD_DIR%/boot.elf %OUT_DIR%/boot.bin

:: Use starfire tool to create the out file
wsl.exe -- bash -lc "cd ../Tools/spl_tool && ./spl_tool -c -f ../../gameos/%OUT_DIR%/boot.bin"
