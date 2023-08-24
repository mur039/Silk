#include <system.h>
#include <gdt.h>
extern uint32_t kernel_end;
extern uint32_t stack_bottom;
extern void flush_tss();
/* Our GDT, with 3 entries, and finally our special GDT pointer */
struct gdt_entry gdt[6]; //NULL COde DATA usrCode usrData tss
struct gdt_ptr gp;

/* This will be a function in start.asm. We use this to properly
*  reload the new segment registers */
//extern void gdt_flush();

/* Setup a descriptor in the Global Descriptor Table */
void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char flags){

    /* Setup the descriptor base address */
    gdt[num].base_low = (base & 0xFFFFFF);
    gdt[num].base_high = (base >> 24) & 0xFF;

    /* Setup the descriptor limits */
    gdt[num].limit_low =  (limit & 0xFFFF);
    gdt[num].limit_high = (limit & 0xF0000) >> 16;

    /* Finally, set up the granularity and access flags */
    gdt[num].accessed = 0; //işlemci eriştiği zaman 1 yapıyo zaten
    gdt[num].read_write = (access & 0b00000010) >> 1; 
    gdt[num].conforming_expand_down = (access & 0b00000100) >> 2; 
    gdt[num].code = (access & 0b00001000) >> 3; 
    gdt[num].code_data_segment = (access & 0b00010000) >> 4; 
    gdt[num].DPL = (access & 0b01100000) >> 6; 
    gdt[num].present = (access & 0b10000000) >> 7; 
    
    gdt[num].available = 0;
    gdt[num].long_mode = (flags & 0b0010) >> 1;
    gdt[num].big = (flags & 0b0100) >> 2;
    gdt[num].gran = (flags & 0b1000) >> 3;
    
}

/* Should be called by main. This will setup the special GDT
*  pointer, set up the first 3 entries in our GDT, and then
*  finally call gdt_flush() in our assembler file in order
*  to tell the processor where the new GDT is and update the
*  new segment registers */
void gdt_install(){
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
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xC); 
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xC); ///* The third entry is our Data Segment.
    
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA ,0xC); //user mode code
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2 ,0xC); //user mode data
    /* Flush out the old GDT and install the new changes! */
    //write_tss(5);

    gdt_flush();
    //flush_tss();
}

tss_entry_t tss_entry;
void write_tss(int index){
    struct gdt_entry * g = &gdt[index];
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry);
    
    g->limit_low = limit & 0xFFFF;
    g->limit_high = (limit & 0xF0000) >> 16;
    g->base_low = base;
    g->base_high = (base >> 24) & 0xFF;

    g->accessed = 1;// code_segment = 0, 1 TSS, 0 LDT
    g->read_write = 0; //tss, 1 busy, 0 not busy
    g->conforming_expand_down = 0; // 0 for tss
    g->code = 1; //tss 1 32-bit, 0 16-bit
    g->code_data_segment = 0; //tss/ldt
    g->DPL = 0; // olmalı
    g->present = 1;
    g->available = 0; //tss 0
    g->long_mode = 0;
    g->big = 0; // öle
    g->gran = 0; //limit in bytes

    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.ss0  = 0x10;
    tss_entry.esp0 = (uint32_t)(&stack_bottom) + 8192;
    return;
}




