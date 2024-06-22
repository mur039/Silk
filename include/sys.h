#ifndef __SYS_H_
#define __SYS_H_
#include <stdint.h>
#include <str.h>
#include <uart.h>

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
 #endif