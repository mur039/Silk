#include <gdt.h>

struct gdt_entry gdt[6]; //NULL COde DATA usrCode usrData Tss
struct gdt_ptr gp;
tss_entry_t tss_entry = {.cr3 = 0};

extern uint32_t kernel_syscallstack[128];


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
    gdt_flush();
    /* Flush out the old GDT and install the new changes! */
    write_tss(&gdt[5]);
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
 

        typedef struct  {
	        unsigned int limit_low              : 16;
	        unsigned int base_low               : 24;
	        unsigned int accessed               :  1;
	        unsigned int read_write             :  1; // readable for code, writable for data
	        unsigned int conforming_expand_down :  1; // conforming for code, expand down for data
	        unsigned int code                   :  1; // 1 for code, 0 for data
	        unsigned int code_data_segment      :  1; // should be 1 for everything but TSS and LDT
	        unsigned int DPL                    :  2; // privilege level
	        unsigned int present                :  1;
	        unsigned int limit_high             :  4;
	        unsigned int available              :  1; // only used in software; has no effect on hardware
	        unsigned int long_mode              :  1;
	        unsigned int big                    :  1; // 32-bit opcodes for code, uint32_t stack for data
	        unsigned int gran                   :  1; // 1 to use 4k page addressing, 0 for byte addressing
	        unsigned int base_high              :  8;
        } __attribute__((packed)) gdt_entry_bits;



    gdt_entry_bits * b = (void *)g;

    
    b->limit_low = limit;
	b->base_low = base;
	b->accessed = 1; // With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
	b->read_write = 0; // For a TSS, indicates busy (1) or not busy (0).
	b->conforming_expand_down = 0; // always 0 for TSS
	b->code = 1; // For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
	b->code_data_segment=0; // indicates TSS/LDT (see also `accessed`)
	b->DPL = 0; // ring 0, see the comments below
	b->present = 1;
	b->limit_high = (limit & (0xf << 16)) >> 16; // isolate top nibble
	b->available = 0; // 0 for a TSS
	b->long_mode = 0;
	b->big = 0; // should leave zero according to manuals.
	b->gran = 0; // limit is in bytes, not pages
	b->base_high = (base & (0xff << 24)) >> 24; //isolate top byte


	// Ensure the TSS is initially zero'd.
	memset(&tss_entry, 0, sizeof tss_entry);
 
	tss_entry.ss0  = (2 * 8) | 0 ;  // Set the kernel stack segment.
	tss_entry.esp0 = 0; // Set the kernel stack pointer.
    tss_entry.cs = 0x0b;
    tss_entry.ds = 0x13;
    tss_entry.es = 0x13;
    tss_entry.fs = 0x13;
    tss_entry.gs = 0x13;
    tss_entry.ss = 0x13;
    
	//note that CS is loaded from the IDT entry and should be the regular kernel code segment
}
 
void set_kernel_stack(uint32_t stack) { // Used when an interrupt occurs
	tss_entry.esp0 = stack;
}
