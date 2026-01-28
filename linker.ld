OUTPUT_ARCH(riscv)
ENTRY(_entry)

SECTIONS
{
  /*
   * jh7110 BootROM loads flash into L2 cach at starting address 0x08000000
   */
  . = 0x08000000;
  __start = .;

  .text : {
    ./build/entry.o(_entry)
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

  ASSERT(. - __start <= 0x00200000, "error: binary file is larger then 2MB")
  PROVIDE(end = .);
}
