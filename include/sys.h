#ifndef __SYS_H_
#define __SYS_H_
#include <stdint.h>
#include <str.h>
#include <uart.h>


extern void enable_interrupts();
extern void disable_interrupts();

#define align2_4kB(pntr) (void *)(((uint32_t)pntr + 4095) & ~4095)

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );

}


static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline void flush_tlb(){
    asm volatile(
        "movl %cr3,%eax\n\t"
	    "movl %eax,%cr3\n\t"
    );
    return;
}

__attribute__((noreturn)) static inline void halt(){
    uart_print(COM1, "\r\nSystem Halted.\r\n");
    disable_interrupts();    
    asm volatile(
        "hlt\n\t"
    );
    //somehow returns?
    for(;;);
}
 #endif