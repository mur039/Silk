#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <system.h>
#include <vga.h>
#include <gdt.h>
#include <idt.h>
#include <isrs.h>
#include <irq.h>


int main(){
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    initVGATerm();
    sti();
    //__asm__  ("div %0" :: "r"(0)); division by zero exceptoin
    //memsetw(((unsigned short *)0xb8000), (( 15 << 4| 0 ) << 8 ) | 'A', 1);
    for(;;);

    return 0;
}