#ifndef _ALLOC_H
#define _ALLOC_H
#include <system.h>
#include <stdint.h>

extern void * kmalloc(uint32_t size);
#define alloca(size); __asm__("movl %esp, %eax\n\tsubl %1, %eax" : : "0"(size) : );
/*asm( "code" : outputs : inputs : clobbers);*/




#endif