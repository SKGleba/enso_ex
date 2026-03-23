OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(start)

SECTIONS
{
  . = 0x51f00000;
  .text   : { *(.text.bootstart) *(.text   .text.*   .gnu.linkonce.t.*) *(.sceStub.text.*) }
  .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .data   : ALIGN(4) { *(.data   .data.*   .gnu.linkonce.d.*) }
  .bss    : ALIGN(4) 
  {
    __bss_start__ = .;
    *(.bss    .bss.*    .gnu.linkonce.b.*)
    *(COMMON)
    __bss_end__ = .;
  }
}
