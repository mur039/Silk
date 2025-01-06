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



void syscall_open(struct regs * r);
void syscall_exit(struct regs * r);
void syscall_read(struct regs * r);
void syscall_write(struct regs * r);
void syscall_close(struct regs * r);
void syscall_lseek(struct regs * r);
void syscall_execve(struct regs * r);
void syscall_fstat(struct regs * r);
void syscall_fork(struct regs * r);
void syscall_dup2(struct regs * r);
void syscall_getpid(struct regs * r);
void syscall_pipe(struct regs * r);
void syscall_wait4(struct regs * r);
void syscall_mmap(struct regs * r);
void syscall_kill(struct regs * r);
void syscall_getcwd(struct regs *r );
void syscall_chdir(struct regs *r );
void syscall_sysinfo(struct regs *r );



#include <process.h>
int close_for_process(pcb_t * process, int fd);
int32_t open_for_process(pcb_t * process, const char * path, int mode );


#endif