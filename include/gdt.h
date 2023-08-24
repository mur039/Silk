#ifndef _GDT_H
#define _GDT_H
struct gdt_entry
{
    uint32_t limit_low:16;
    uint32_t base_low:24;
    //accesses start
    uint32_t accessed:1;
    uint32_t read_write:1; //readable for code, writable for data
    uint32_t conforming_expand_down:1; //conforming for code, expand down for data
    uint32_t code:1; //code 1, data 0     
    uint32_t code_data_segment:1; //should be 1 for all but tss and ldt
    uint32_t DPL :2; //privilege level
    uint32_t present:1;
    //accesses end

    uint32_t limit_high:4;

    //flags start
    uint32_t available:1; //only in software no effect in hardware //reserver?
    uint32_t long_mode:1; // 1 64-bit code segment long mode, whent set big should be zero
    uint32_t big :1; // 0 16-bit protected mode, 1 32-bit 
    uint32_t gran:1; //size = limit*granularity, 0 1-byte, 1 4kB
    //flags end
    
    uint32_t base_high:8;
} __attribute__((packed));

/* Special pointer which includes the limit: The max bytes
*  taken up by the GDT, minus 1. Again, this NEEDS to be packed */
struct gdt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct tss_entry_struct {
	uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __packed;
 
typedef struct tss_entry_struct tss_entry_t;



extern void gdt_flush();
void gdt_install(void);
void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char flags);
void write_tss(int index);

#endif