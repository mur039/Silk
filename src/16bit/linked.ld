

ENTRY(_16start)
SECTIONS {
	. = 0x00007C00;
	.text :
	{
		*(.text)
	}

	.rodata :  
	{
		*(.rodata*)
	}

	.data :
	{
		*(.data)
	}

	.bss ALIGN (0x4) :
	{
		*(COMMON)                               
		*(.bss)                        
        
	}
	
}
