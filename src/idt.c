#include <idt.h>
struct idt_entry idt[256] __attribute__((aligned(0x10)));

struct idt_ptr idtp;

//inthe boot
void idt_load(){
    asm volatile("lidt (idtp)");
    return;
}

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags)
{
    struct idt_entry * entry = &idt[num];
    entry->base_lo = base & 0xFFFF;
    entry->base_hi = (base >> 16)& 0xFFFF;
    entry->always0 = 0;
    entry->sel = sel;
    entry->flags = flags;
    return;
}
/* Installs the IDT */
void idt_install()
{
    /* Sets the special IDT pointer up, just like in 'gdt.c' */
    idtp.limit = (sizeof (struct idt_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;
    memset(&idt, 0, sizeof(struct idt_entry) * 256); //?? ** 

    /* Add any new ISRs to the IDT here using idt_set_gate */
    idt_load();
}
	