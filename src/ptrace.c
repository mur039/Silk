#include <ptrace.h>
#include <syscalls.h>
#include <process.h>




/*long ptrace(enum __ptrace_request op, pid_t pid, void *addr, void *data);*/
void syscall_ptrace(struct regs* r){

    unsigned long op = (unsigned long)r->ebx;
    pid_t pid = (pid_t)r->ecx;
    void* addr = (void*)r->edx;
    void* data = (void*)r->esi;

    fb_console_printf("%s: %x %d %x %x\n", __func__, op, pid, addr, data);

    r->eax = -ENOSYS;
    return;
}


int ptrace_initialize(){

    install_syscall_handler(SYSCALL_PTRACE, syscall_ptrace);
}