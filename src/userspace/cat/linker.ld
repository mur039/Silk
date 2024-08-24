ENTRY(_start)

MEMORY {

  ram (rwx) : ORIGIN = 0x11e000, LENGTH = 0xFFF 
}


SECTIONS {

	.text :
	{
		*(.text)                                /* all text sections from all files */
	 } > ram

	.rodata :
	{
		*( .rodata*)                             /* all read-only data sections from all files */
	} >ram

	.data :
	{
		*(.data)                                /* all data sections from all files */
	} >ram

	.bss :
	{
		*(COMMON)                               /* all COMMON sections from all files */
		*(.bss)                                 /* all bss sections from all files */
	} >ram
	
}
