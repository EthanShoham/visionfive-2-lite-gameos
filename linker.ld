MEMORY { .sram : ORIGIN = 0x08000000, LENGTH = 0x00200000 }

OUTPUT_ARCH(riscv)
ENTRY(_entry)

SECTIONS
{
  . = ORIGIN(.sram);

  .text : {
    KEEP(*(.text.entry))
    *(.text .text.*)
  } > .sram

  .rodata : {
    *(.rodata .rodata.*)
  } > .sram

  /* this will set the first 4 bits to 0 making it 16 byte aligned address */
  . = (ORIGIN(.sram) + LENGTH(.sram)) & ~0xF;
  PROVIDE(__stack_top = .);
}
