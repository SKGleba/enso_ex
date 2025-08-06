OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(start)

SECTIONS
{
  . = 0x51e00000;
  .text   : { *(.text.start) *(.text   .text.*   .gnu.linkonce.t.*) *(.sceStub.text.*) }
  .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .data   : { *(.data   .data.*   .gnu.linkonce.d.*) *(.bss    .bss.*    .gnu.linkonce.b.*) *(COMMON) }
  /* .bss    : { *(.bss    .bss.*    .gnu.linkonce.b.*) *(COMMON) } */
}

/* ASSERT(SIZEOF(.bss) == 0, ".bss section is used!") */