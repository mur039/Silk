ENTRY(_start)

MEMORY {

  ram (rwx) : ORIGIN = 0x10000, LENGTH = 0x2FFF 
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
		_bss_start = .;

		*(COMMON)                               /* all COMMON sections from all files */
		*(.bss)

		_bss_end = .;                         /* all bss sections from all files */
	} >ram
	
	
}
