#include <gdt.h>

struct gdt_entry gdt[6]; //NULL COde DATA usrCode usrData Tss
struct gdt_ptr gp;
tss_entry_t tss_entry = {.cr3 = 0};


void gdt_install(void){
        /* Setup the GDT pointer and limit */
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = (unsigned int)&gdt;

    /* Our NULL descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* The second entry is our Code Segment. The base address
    *  is 0, the limit is 4GBytes, it uses 4KByte granularity,
    *  uses 32-bit opcodes, and is a Code Segment descriptor.
    *  Please check the table above in the tutorial in order
    *  to see exactly what each value means */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* The third entry is our Data Segment. It's EXACTLY the
    *  same as our code segment, but the descriptor type in
    *  this entry's access byte says it's a Data Segment */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_set_gate(3, 0x0, 0xFFFFFFFF, 0xFA ,0xCF); //user mode code
    gdt_set_gate(4, 0x0, 0xFFFFFFFF, 0xF2 ,0xCF); //user mode data
    write_tss(&gdt[5]);
    /* Flush out the old GDT and install the new changes! */
    gdt_flush();
    flush_tss();
}


void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran){
    /* Setup the descriptor base address */
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    /* Setup the descriptor limits */
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    /* Finally, set up the granularity and access flags */
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;

}

void write_tss(struct gdt_entry *g) {
	// Compute the base and limit of the TSS for use in the GDT entry.
	uint32_t base = (uint32_t) &tss_entry;
	uint32_t limit = sizeof(tss_entry);
 
	// Add a TSS descriptor to the GDT.
	g->limit_low = limit;
	g->base_low = base;
    g->access = 0b10001001;

	// g->accessed = 1; // With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
	// g->read_write = 0; // For a TSS, indicates busy (1) or not busy (0).
	// g->conforming_expand_down = 0; // always 0 for TSS
	// g->code = 1; // For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
	// g->code_data_segment=0; // indicates TSS/LDT (see also `accessed`)
	// g->DPL = 0; // ring 0, see the comments below
	// g->present = 1;

    g->granularity = ((limit & (0xf << 16)) >> 16) & 0xF; 
	// g->limit_high = (limit & (0xf << 16)) >> 16; // isolate top nibble
	// g->available = 0; // 0 for a TSS
	// g->long_mode = 0;
	// g->big = 0; // should leave zero according to manuals.
	// g->gran = 0; // limit is in bytes, not pages

	g->base_high = (base & (0xff << 24)) >> 24; //isolate top byte
 
	// Ensure the TSS is initially zero'd.
	memset(&tss_entry, 0, sizeof tss_entry);
 
	tss_entry.ss0  = 2;  // Set the kernel stack segment.
	tss_entry.esp0 = 0; // Set the kernel stack pointer.
	//note that CS is loaded from the IDT entry and should be the regular kernel code segment
}
 
void set_kernel_stack(uint32_t stack) { // Used when an interrupt occurs
	tss_entry.esp0 = stack;
}
