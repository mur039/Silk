#ifndef __IDT_H_
#define __IDT_H_
#include <stdint.h>
#include <str.h>

typedef enum{  
	IDT_FLAG_GATE_TASK   = 0b0101, //0x05
	IDT_FLAG_16_BIT_INT  = 0b0110, //0x06
	IDT_FLAG_16_BIT_TRAP = 0b0111, //0x07
	IDT_FLAG_32_BIT_INT  = 0b1110, //0x0E #disable int
	IDT_FLAG_32_BIT_TRAP = 0b1111, //0x0F #doesnt

	IDT_FLAG_RING0       = (1 << 5),
	IDT_FLAG_RING1       = (2 << 5),
	IDT_FLAG_RING2       = (3 << 5),
	IDT_FLAG_RING3       = (4 << 5),

	IDT_FLAG_PRESENT	 = 0b10000000 //0x80
} IDT_FLAGS;


struct idt_entry
{
    unsigned short base_lo;
    unsigned short sel;        /* Our kernel segment goes here! */
    unsigned char always0;     /* This will ALWAYS be set to 0! */
    unsigned char flags;       /* Set using the above table! */
    unsigned short base_hi;
} __attribute__((packed));

struct idt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags);
void idt_install();
void idt_load();

#endif