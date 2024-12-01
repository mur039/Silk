#include <syscalls.h>
#include <isr.h>
#include <pmm.h>

void (*syscall_handlers[64])(struct regs *r);


void unkown_syscall(struct regs *r){
    uart_print(COM1, "Unkown syscall %x\r\n", r->eax);
    r->eax = -1;
}

extern void syscall_stub();
void initialize_syscalls(){
    idt_set_gate(0x80, (unsigned)syscall_stub, 0x08, 0xEE);//0x8E); //syscall
    for (int i = 0; i < 64; i++)
    {
        syscall_handlers[i] = unkown_syscall;
    }
    
}

int install_syscall_handler(uint32_t syscall_number, void (*syscall_handler)(struct regs *r))
{
    if(syscall_number >= 64){
        return -1; //out of index
    }

    syscall_handlers[syscall_number] = syscall_handler;
    return 0;
}


void syscall_handler(struct regs *r){
    if(r->eax > 64){
        uart_print(COM1, "Syscall , %x, out of range!. Halting", r->eax);
        halt();
    }

    // should switch back to the kernel directory too
    // u32 old_cr3;
    // asm volatile ( "movl %%cr3, %0"
    //    : "=a"(old_cr3)
    //    :  
    //    :
    //    );

    // asm volatile ( "movl %0, %%cr3"
    // : 
    // : "a"(kdir_entry) 
    // :
    // );

    // flush_tlb();
    syscall_handlers[r->eax](r);

    // //move back
    // asm volatile ( "movl %0, %%cr3"
    //    : 
    //    : "a"(old_cr3) 
    //    :
    //    );
    return;
}

