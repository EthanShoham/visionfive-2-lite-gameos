KERNEL_START = DEFINED(KERNEL_START) ? KERNEL_START : 0x40000000;

OUTPUT_ARCH(riscv)
ENTRY(_entry)

SECTIONS
{
  . = KERNEL_START;
  kernel_begining = .;

  .text : {
    KEEP(*(.text.entry))
    *(.text .text.*)
    . = ALIGN(0x1000);
    _trampoline = .;
    KEEP(*(trampsec))
    . = ALIGN(0x1000);
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger then one page");
    PROVIDE(etext = .);
  }

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*)
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*)
    . = ALIGN(16);
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*)
    . = ALIGN(16);
    *(.bss .bss.*)
  }

  ASSERT(. - kernel_begining <= 0x00200000, "error: binary file is larger then 2MB")
  kernel_end = .;
}
