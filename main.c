#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <system.h>
#include <vga.h>
#include <gdt.h>
#include <idt.h>
#include <isrs.h>
#include <irq.h>
#include <timer.h>
#include <kb.h>
#include <uart.h>
#include <k_heap.h>
#include <paging.h>

extern uint32_t _binary_usr_program_end[];
extern uint32_t _binary_usr_program_size;
extern uint32_t _binary_usr_program_start[];

int test(uint32_t thing){
    return thing;
}

int main(){
    initVGATerm();
    serialInit(COM1, 384000);//max possible 

    printf("Meraba\n");
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    init_paging();

    irq_install_handler(1, keyboard_handler);
    irq_install_handler(4, uart_handler);
    sti();
    //map_page((void *)0xb8000, (void *)0x200000, 3);
    memcpy((void *)0x200000, _binary_usr_program_start,  _binary_usr_program_end - _binary_usr_program_start);

    
    
    

    /*
    [esp + 16]  ss      ; the stack segment selector we want for user mode
    [esp + 12]  esp     ; the user mode stack pointer
    [esp +  8]  eflags  ; the control flags we want to use in user mode
    [esp +  4]  cs      ; the code segment selector
    [esp +  0]  eip     ; the instruction pointer of user mode code to execute
    */

    //lets try user mode
    asm volatile("mov $0x13, %eax"); 
    asm volatile("mov %eax, %ds"); 
    asm volatile("mov %eax, %es");
    asm volatile("mov %eax, %gs"); 
    asm volatile("mov %eax, %fs"); 
    
    asm volatile("push $0x13"); //ss 
    asm volatile("push $201000"); //sp
    asm volatile("push $512"); //eflags
    asm volatile("push $0xB"); //cs 
    asm volatile("push $200000"); //eip
    asm volatile("sysret");
   //__asm__  ("div %0" :: "r"(0)); //division by zero exceptoin
    for(;;);

    return 0;
}
