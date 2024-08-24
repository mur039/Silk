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

enum syscall_numbers{
    SYSCALL_READ = 0,
    SYSCALL_WRITE = 1,
    SYSCALL_OPEN = 2,
    SYSCALL_CLOSE = 3,
    SYSCALL_LSEEK = 4,
    SYSCALL_FSTAT = 5,
    SYSCALL_EXECVE = 59,
    SYSCALL_EXIT = 60
};

void initialize_syscalls();
void unkown_syscall(struct regs *r);
int install_syscall_handler(uint32_t syscall_number, void (*syscall_handler)(struct regs *r));
void syscall_handler(struct regs *r);
#endif