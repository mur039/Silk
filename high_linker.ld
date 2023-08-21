    ENTRY(start)                /* the name of the entry label */

SECTIONS{
    . = 0xC0100000;          /* the code should be relocated to 3G + 1 MB */

    .text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000) /* align at 4 KB */
    {
      code = .;
      *(.text)             /* all text sections from all files */
    }

    .rodata ALIGN (0x1000) : AT(ADDR(.rodata) - 0xC0000000)
    {
      *(.rodata*)          /* all read-only data sections from all files */
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000)
    {
      data = .;
      *(.data)             /* all data sections from all files */
    }

    .bss ALIGN (0x1000) : AT(ADDR(.bss) - 0xC0000000)
    {
        bss = .;
        *(COMMON)            /* all COMMON sections from all files */
        *(.bss)              /* all bss sections from all files */
    }
  end = .;
  kernel_end = end;
}
