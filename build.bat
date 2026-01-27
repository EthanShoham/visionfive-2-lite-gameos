:: march = Target RISC-V ISA (which instructions the CPU supports)
:: rv64  = 64-bit RISC-V
:: i     = base integer instruction set (mandatory)
:: m     = multiply/divide instructions
:: a     = atomic instructions
:: c     = compressed 16-bit instructions (smaller code size)
::
:: mabi  = ABI (how functions pass arguments, use registers, and lay out the stack)
:: lp64  = 64-bit longs and pointers, integer-only ABI (no FPU usage)

riscv-none-elf-gcc -c entry.S -o entry.o ^
    -march=rv64imac_zicsr -mabi=lp64


:: -Os              = optimize for size (important for SRAM-limited SPL)
:: -ffreestanding   = tell compiler there is NO OS or standard runtime
:: -fno-builtin     = prevent GCC from inserting calls to libc functions (memcpy, memset, etc.)
:: -fno-pic         = disable position-independent code (no GOT / relocations)
:: -nostdlib        = do not link against libc or libgcc
:: -nostartfiles    = do not use default C runtime startup (crt0)
:: -march / -mabi   = same ISA and ABI as assembly code (must match!)

riscv-none-elf-gcc -c start.c -o start.o ^
    -Os -ffreestanding -fno-builtin -fno-pic ^
    -nostdlib -nostartfiles ^
    -march=rv64imac -mabi=lp64


:: Link stage:
:: -nostdlib        = do not pull in libc or libgcc
:: -nostartfiles    = do not add default startup objects
:: -Wl,-T,linker.ld = use custom linker script (controls load address, memory layout)
:: -Wl,--gc-sections= garbage-collect unused code/data sections (reduces SPL size)

riscv-none-elf-gcc -o boot.elf ^
    entry.o ^
    start.o ^
    -nostdlib -nostartfiles ^
    -Wl,-m,elf64lriscv -Wl,-T,linker.ld -Wl,--gc-sections


:: Convert ELF file to raw binary
:: -O binary = strip ELF headers and metadata
:: Result is a flat image suitable for ROM / SD / SPI loading

riscv-none-elf-objcopy -O binary boot.elf boot.bin
