#ifndef __PTRACE_H__
#define __PTRACE_H__

#include <syscalls.h>

int ptrace_initialize();
void syscall_ptrace(struct regs* r);





#endif