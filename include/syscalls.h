#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <stdint.h>
#include <sys.h>
#include <isr.h>

/* convention
retvalue = 	eax
syscall number = eax	    
arg0 = ebx	
arg1 = ecx	
arg2 = edx	
arg3= esi	
arg4 = edi	
arg5 = ebp
*/

#include <sys/syscall.h>

void initialize_syscalls();
void unkown_syscall(struct regs *r);
int install_syscall_handler(uint32_t syscall_number, void (*syscall_handler)(struct regs *r));
void syscall_handler(struct regs *r);
#endif