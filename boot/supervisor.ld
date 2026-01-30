/*
* jh7110 BootROM loads flash into L2 cach at starting address 0x08000000
*/
STARTING_ADDRESS = 0x40000000;
ALIGNMENT = 16;
STACK_SIZE = 0x1000; /* stack for supervisor core */
MAX_L2_CACHE_SIZE = 0x00200000;

OUTPUT_ARCH(riscv)
ENTRY(_supervisor)

SECTIONS
{
  . = STARTING_ADDRESS;
  supervisor_begining = .;

  .text : {
    KEEP(*(.text.supervisor))
    *(.text .text.*)
    PROVIDE(etext = .);
  }

  .rodata : {
    . = ALIGN(ALIGNMENT);
    *(.srodata .srodata.*)
    . = ALIGN(ALIGNMENT);
    *(.rodata .rodata.*)
  }

  .data : {
    . = ALIGN(ALIGNMENT);
    *(.sdata .sdata.*)
    . = ALIGN(ALIGNMENT);
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(ALIGNMENT);
    bss_start = .;
    *(.sbss .sbss.*)
    . = ALIGN(ALIGNMENT);
    *(.bss .bss.*)
    . = ALIGN(ALIGNMENT);
    bss_end = .;
  }

  .stack : {
    . = ALIGN(ALIGNMENT);
    stack_bottom = .;
    . = . + STACK_SIZE;
    PROVIDE(stack_top = .);
  }

  ASSERT(. - supervisor_begining <= MAX_L2_CACHE_SIZE, "error: binary file is larger then 2MB")
  supervisor_end = .;
}
