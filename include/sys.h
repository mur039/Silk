#ifndef __SYS_H_
#define __SYS_H_

#include <stdint.h>
#include <stdint-gcc.h>
#include <str.h>





typedef unsigned long long int u64;
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

static inline void outw(uint16_t port, uint16_t val)
{
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );

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

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ( "inw %1, %0"
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





//CR0 fields



static inline uint32_t get_cr0(){
    uint32_t ret;
    asm volatile ( "mov %%cr0, %0" : "=r"(ret) );
    return ret;
}

//will return old one and replace it
static inline uint32_t set_cr0(uint32_t cr0){
    uint32_t ret = get_cr0();
       __asm__ volatile (
        "mov %0, %%cr0"
        :
        : "r"(cr0)
        : "memory"
    );
    return ret;
}

static inline uint32_t get_cr3(){
    uint32_t ret;
    asm volatile ( "mov %%cr3, %0" : "=r"(ret) );
    return ret;
}

//will return old one and replace it
static inline uint32_t set_cr3(u32 cr3){
    uint32_t ret = get_cr3();
       __asm__ volatile (
        "mov %0, %%cr3"
        :
        : "r"(cr3)
        : "memory"
    );
    return ret;
}


static inline uint32_t get_cr4(){
    uint32_t ret;
    asm volatile ( "mov %%cr4, %0" : "=r"(ret) );
    return ret;
}

//will return old one and replace it
static inline uint32_t set_cr4(u32 cr4){
    uint32_t ret = get_cr4();
       __asm__ volatile (
        "mov %0, %%cr4"
        :
        : "r"(cr4)
        : "memory"
    );
    return ret;
}







static inline void flush_tlb(){
    asm volatile(
        "movl %cr3,%eax\n\t"
	    "movl %eax,%cr3\n\t"
    );
    return;
}



extern void uart_print(int port, const char* source, ...);

__attribute__((noreturn)) static inline void halt(){
    uart_print(0x3f8, "\r\nSystem Halted.\r\n");
    disable_interrupts();    
    asm volatile(
        "hlt\n\t"
    );
    //somehow returns?
    for(;;);
}


static inline void idle(){
    enable_interrupts();    
    asm volatile(
        "hlt\n\t"
    );
    // disable_interrupts();
    return;
}


extern void io_wait();




#define GET_BIT(value, bit) ((value >> bit) & 1)

static inline const char * find_next(const char * src, char c){
    const char * phead = src;
    while(*(phead++) != c  && (int)(phead - src) <= 512); //lets impose some arbitrary limit
    return phead;
}

static inline void stack_push(unsigned int ** stack, unsigned int value){
    unsigned int val = value;
    stack[0] -= 1;
    stack[0][0] = val;
    return;
}



 #endif