#ifndef _ISR_H
#define _ISR_H


/* This defines what the stack looks like after an ISR was running */
struct regs
{
    unsigned int gs, fs, es, ds;      /* pushed the segs last */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
    unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
};




//eflag bits
#define EFLAGS_INTERRUPT_ENABLE_MASK 0x200
#define EFLAGS_TRAP_MASK 0x100
#define EFLAGS_CPU_ID_MASK 0x00200000

void isrs_install();
void fault_handler(struct regs *r);
void dump_registers(struct regs *r);
#endif