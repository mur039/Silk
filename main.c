#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <system.h>
#include <alloc.h>
#include <vga.h>
#include <gdt.h>
#include <idt.h>
#include <isrs.h>
#include <irq.h>
#include <timer.h>
#include <kb.h>
#include <uart.h>

int main(){
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    serialInit(COM1, 38400);//max possible 
    irq_install_handler(1, keyboard_handler);
    irq_install_handler(4, uart_handler);
    initVGATerm();
    //sti();
    alloca(25);
   //__asm__  ("div %0" :: "r"(0)); //division by zero exceptoin
     
    
    for(;;){
    }

    return 0;
}
