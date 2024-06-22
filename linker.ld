ENTRY(_start)


SECTIONS {

	. = 0x00100000;                                 /* the code should be loaded at 1 MB */

    .multiboot : {
        boot.o( .text .rodata .data COMMON .bss)
		lkmain.o (.text .rodata .data COMMON .bss)
				
    }


    . += 0xC0000000;                                 /* the code should be not loaded at 1 MB */
	kernel_start = .;
	kernel_phy_start = . - 0xC0000000;

	.text ALIGN (0x1000) : AT (ADDR (.text) - 0xC0000000)
	{
		*(EXCLUDE_FILE(boot.o lkmain.o) .text)                                /* all text sections from all files */
	}

	.rodata ALIGN (0x1000) : AT (ADDR (.rodata) - 0xC0000000)
	{
		*(EXCLUDE_FILE(boot.o lkmain.o) .rodata*)                             /* all read-only data sections from all files */
	}

	.data ALIGN (0x1000) : AT (ADDR (.data) - 0xC0000000)
	{
		*(EXCLUDE_FILE(boot.o lkmain.o) .data)                                /* all data sections from all files */
	}

	.bss ALIGN (0x1000) : AT (ADDR (.bss) - 0xC0000000)
	{
		*(COMMON)                               /* all COMMON sections from all files */
		*(.bss)                                 /* all bss sections from all files */
	}
	
	kernel_end = .;
	kernel_phy_end = . - 0xC0000000;
}
