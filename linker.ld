MEMORY { .sram : ORIGIN = 0x08000000, LENGTH = 0x00040000 }

OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
  . = ALIGN(4);
  .text : {
    start.o (.text)
  } > .sram
}
