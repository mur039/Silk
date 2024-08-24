#ifndef __SYS_H_
#define __SYS_H_
#include <stdint.h>
#include <str.h>
#include <uart.h>

#ifndef NULL
#define NULL (void *)0
#endif


typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef signed int   i32;
typedef signed short i16;
typedef signed char  i8;





extern void enable_interrupts();
extern void disable_interrupts();

#define align2_4kB(pntr) (void *)(((uint32_t)pntr + 4095) & ~4095)

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );

}


static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );

}



static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ( "inl %1, %0"
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
#define GET_BIT(value, bit) ((value >> bit) & 1)

static inline const char * find_next(const char * src, char c){
    const char * phead = src;
    while(*(phead++) != c  && (int)(phead - src) <= 512); //lets impose some arbitrary limit
    return phead;
}

 #endif