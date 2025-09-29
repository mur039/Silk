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
void syscall_getdents(struct regs *r );
void syscall_getcwd(struct regs *r );
void syscall_chdir(struct regs *r );
void syscall_mkdir(struct regs *r );
void syscall_sysinfo(struct regs *r );
void syscall_mount(struct regs *r);
void syscall_unlink(struct regs * r);
void syscall_ioctl(struct regs * r);
void syscall_pivot_root(struct regs *r);
void syscall_fcntl(struct regs *r);
void syscall_pause(struct regs *r);
void syscall_getppid(struct regs *r);
void syscall_getpgrp(struct regs *r);
void syscall_setpgid(struct regs *r);
void syscall_setsid(struct regs* r);
void syscall_nanosleep(struct regs* r);
void syscall_sched_yield(struct regs* r);
void syscall_poll(struct regs* r);
void syscall_getuid(struct regs* r);
void syscall_getgid(struct regs* r);
void syscall_setuid(struct regs* r);
void syscall_setgid(struct regs* r);
void syscall_geteuid(struct regs* r);
void syscall_getegid(struct regs* r);
void syscall_ftruncate(struct regs* r);


#define MAX_SYSCALL_NUMBER 256
extern void (*syscall_handlers[MAX_SYSCALL_NUMBER])(struct regs *r);


#include <process.h>
int close_for_process(pcb_t * process, int fd);


#define O_RDONLY    0x00000001
#define O_WRONLY    0x00000002
#define O_RDWR      0x00000004
#define O_APPEND    0x00000008
#define O_CREAT     0x00000010
#define O_DSYNC     0x00000020
#define O_EXCL      0x00000040
#define O_NOCTTY    0x00000080
#define O_NONBLOCK  0x00000100
#define O_RSYNC     0x00000200
#define O_SYNC      0x00000400
#define O_TRUNC     0x00000800
int32_t open_for_process(pcb_t * process,  char * path, int flags, int mode );

//for fd's
#define EINVAL 1
#define EAGAIN 2
#define EBADF  3 
#define EMFILE 4 //peprocess file limit
#define EINTR 5
#define ENOBUFS 6
#define EADDRINUSE 7
#define ENOTSOCK 8
#define EEXIST 9
#define ENOSYS 10
#define ENOMEM 11
#define ERANGE 12
#define EIO 13
#define EDOM 14
#define EFBIG 15
#define ENOENT 16
#define ENOTDIR 17
#define EACCES 18
#define ENOEXEC 19
#define EWOULDBLOCK 20
#define ENOTTY 21
#define ENOPERM 22
#define EFAULT 23
#define ENXIO 24
#define ENODEV 25
#endif