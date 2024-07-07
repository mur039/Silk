#include <sys.h>
#include <isr.h>
#include <idt.h>

#include <uart.h>
#include <str.h>
 
/* These are function prototypes for all of the exception
*  handlers: The first 32 entries in the IDT are reserved
*  by Intel, and are designed to service exceptions! */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void syscall_stub();

/* This is a very repetitive function... it's not hard, it's
*  just annoying. As you can see, we set the first 32 entries
*  in the IDT to the first 32 ISRs. We can't use a for loop
*  for this, because there is no way to get the function names
*  that correspond to that given entry. We set the access
*  flags to 0x8E. This means that the entry is present, is
*  running in ring 0 (kernel level), and has the lower 5 bits
*  set to the required '14', which is represented by 'E' in
*  hex. */
void isrs_install()
{
    //lets convert them to ring3's
    idt_set_gate( 0, ( unsigned)isr0, 0x08, 0xEE);//0x8E);
    idt_set_gate( 1, ( unsigned)isr1, 0x08, 0xEE);//0x8E);
    idt_set_gate( 2, ( unsigned)isr2, 0x08, 0xEE);//0x8E);
    idt_set_gate( 3, ( unsigned)isr3, 0x08, 0xEE);//0x8E);
    idt_set_gate( 4, ( unsigned)isr4, 0x08, 0xEE);//0x8E);
    idt_set_gate( 5, ( unsigned)isr5, 0x08, 0xEE);//0x8E);
    idt_set_gate( 6, ( unsigned)isr6, 0x08, 0xEE);//0x8E);
    idt_set_gate( 7, ( unsigned)isr7, 0x08, 0xEE);//0x8E);
    idt_set_gate( 8, ( unsigned)isr8, 0x08, 0xEE);//0x8E);
    idt_set_gate( 9, ( unsigned)isr9, 0x08, 0xEE);//0x8E);
    idt_set_gate(10, (unsigned)isr10, 0x08, 0xEE);//0x8E);
    idt_set_gate(11, (unsigned)isr11, 0x08, 0xEE);//0x8E);
    idt_set_gate(12, (unsigned)isr12, 0x08, 0xEE);//0x8E);
    idt_set_gate(13, (unsigned)isr13, 0x08, 0xEE);//0x8E);
    idt_set_gate(14, (unsigned)isr14, 0x08, 0xEE);//0x8E);
    idt_set_gate(15, (unsigned)isr15, 0x08, 0xEE);//0x8E);
    idt_set_gate(16, (unsigned)isr16, 0x08, 0xEE);//0x8E);
    idt_set_gate(17, (unsigned)isr17, 0x08, 0xEE);//0x8E);
    idt_set_gate(18, (unsigned)isr18, 0x08, 0xEE);//0x8E);
    idt_set_gate(19, (unsigned)isr19, 0x08, 0xEE);//0x8E);
    idt_set_gate(20, (unsigned)isr20, 0x08, 0xEE);//0x8E);
    idt_set_gate(21, (unsigned)isr21, 0x08, 0xEE);//0x8E);
    idt_set_gate(22, (unsigned)isr22, 0x08, 0xEE);//0x8E);
    idt_set_gate(23, (unsigned)isr23, 0x08, 0xEE);//0x8E);
    idt_set_gate(24, (unsigned)isr24, 0x08, 0xEE);//0x8E);
    idt_set_gate(25, (unsigned)isr25, 0x08, 0xEE);//0x8E);
    idt_set_gate(26, (unsigned)isr26, 0x08, 0xEE);//0x8E);
    idt_set_gate(27, (unsigned)isr27, 0x08, 0xEE);//0x8E);
    idt_set_gate(28, (unsigned)isr28, 0x08, 0xEE);//0x8E);
    idt_set_gate(29, (unsigned)isr29, 0x08, 0xEE);//0x8E);
    idt_set_gate(30, (unsigned)isr30, 0x08, 0xEE);//0x8E);
    idt_set_gate(31, (unsigned)isr31, 0x08, 0xEE);//0x8E);
    idt_set_gate(0x80, (unsigned)syscall_stub, 0x08, 0xEE);//0x8E); //syscall
    return;
}

/* This is a simple string array. It contains the message that
*  corresponds to each and every exception. We get the correct
*  message by accessing like:
*  exception_message[interrupt_number] */
void dump_registers(struct regs *r){
    uart_print(
	    0x3f8,
         "\r\n"
         "eax : %x | ebx : %x | ecx : %x\r\n"
         "edx : %x | edi : %x | esi : %x\r\n"
         "eip : %x | esp : %x | ebp : %x\r\n"
         ,r->eax, r->ebx, r->ecx
         ,r->edx, r->edi, r->esi
         ,r->eip, r->esp, r->ebp
         );
 }

static const char *exception_messages[] =
{
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bound",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check  (486+)",
    "Machine Check  (Pentium/586+)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved "

};

static const char *pagefault_messages[] =
{
"Supervisor tried to access a non-present page entry.\r\n",
"Supervisory process tried to read a page and caused a protection fault.\r\n",
"Supervisory process tried to write to a non-present page entry.\r\n",
"Supervisory process tried to write a page and caused a protection fault.\r\n",
"User process tried to read a non-present page entry.\r\n",
"User process tried to read a page and caused a protection fault.\r\n",
"User process tried to write to a non-present page entry.\r\n",
"User process tried to write a page and caused a protection fault.\r\n"
};


/* All of our  handling Interrupt Service Routines will
*  point to this function. This will tell us what exception has
*  happened! Right now, we simply halt the system by hitting an
*  endless loop. All ISRs disable interrupts while they are being
*  serviced as a 'locking' mechanism to prevent an IRQ from
*  happening and messing up kernel data structures */
void fault_handler(struct regs *r)
{
    char buffer[72];
    uint32_t message_length = 0;
    /* Is this a fault whose number is from 0 to 31? */
    if (r->int_no < 32)
    {
	dump_registers(r);
        
       switch (r->int_no)
       {
       case 13: //general fault
            message_length = sprintf(buffer, "%x %s\r\n", r->int_no, exception_messages[r->int_no]);
            uart_write(0x3f8, buffer, 1, message_length);
            uart_print(0x3f8, "\r\nSystem Halted.");
            halt(); 
            break;
       case 14: //Page Fault
            uart_print(0x3f8, pagefault_messages[r->err_code & 7]);

            if(0 == (r->err_code & 1)){ //page not present, cr2 gives linear address
                uint32_t cr2_reg;
                asm volatile ( "mov %%cr0, %0" : "=r"(cr2_reg) );
                uart_print(0x3f8, "\nCR2 : [PDE] %x : [PTE] %x -> 0x%x\r\n", cr2_reg >> 22, (cr2_reg >> 12) & 0x3FF , cr2_reg);

            }

            halt();
        break;

       default:
            ;
            message_length =  sprintf(buffer, "%s\r\n", exception_messages[r->int_no]);
            uart_write(0x3f8, buffer, 1, message_length);
            // puts(exception_messages[r->int_no]);
            // puts(" Exception. System Halted!\n");
            break;
       }
        
        // for (;;);
    }
}
		
