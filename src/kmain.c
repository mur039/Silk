#include <multiboot.h>
#include <stdint.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <uart.h>
#include <acpi.h>   
#include <pmm.h>
#include <str.h>
#include <pit.h>
#include <gdt.h>
#include <kb.h>
#include <timer.h>

#define COM1 0x3f8
#define align2_4kB(pntr) (uint32_t *)(((uint32_t)pntr + 4095) & ~4095)
extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t kernel_phy_end;
extern void jump_usermode(uint32_t * func);


// convention
/*
retvalue = 	eax
syscall number = eax	    
arg0 = ebx	
arg1 = ecx	
arg2 = edx	
arg3= esi	
arg4 = edi	
arg5 = ebp
*/
void syscall_handler(struct regs *r){
    uart_print(COM1, "EAX : %x\r\n", r->eax);
    for(;;);
    return;
}

void uart_handler(struct regs *r){
    char c = serial_read(0x3f8);
    uart_print(COM1, "[*]uart_handler : %c\r\n", c);
    return;
}

uint32_t kernel_stack[2048];
uint32_t kernel_syscallstack[128];


 __attribute__((noreturn)) void kmain(multiboot_info_t* mbd){ //high kernel

    // disable_pit();
    init_uart_port(COM1); //COM1
    uart_print(COM1, "Modules found : %x, At : %x\r\n", mbd->mods_count, mbd->mods_addr);
    multiboot_module_t * modules  = (multiboot_module_t *)mbd->mods_addr;

    

    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(struct multiboot_mmap_entry)){

        // mmt->type; //look for 1;
        char buffer[64];
        multiboot_memory_map_t * mmt = (multiboot_memory_map_t *)(mbd->mmap_addr +i);
        sprintf(buffer, "%x...%x : Size:%x -> Type : %s\r\n", 
        mmt->addr_low,
        mmt->addr_low + mmt->len_low,
        mmt->len_low,
        mmt->type == 1?
        "Available" :
        "Reserved"
        );
        uart_write(COM1, buffer, 1, 64);
    }

    //duplicated but anyway
    uart_print(COM1, "Initializing Pyhsical Memory Manager\r\n");

    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(struct multiboot_mmap_entry)){
        multiboot_memory_map_t * mmt = (multiboot_memory_map_t *)(mbd->mmap_addr +i);
        if(mmt->type == 1 && mmt->addr_low == 0x100000) pmm_init(mmt->len_low);
    }
    

    uart_print(COM1, "Installing Global Descriptor Tables\r\n");
    gdt_install();
    
    set_kernel_stack((uint32_t)kernel_syscallstack);
    flush_tss();

    uart_print(COM1, "Installing Interrupt Descriptor Tables\r\n");
    idt_install();

    uart_print(COM1, "Installing ISRs\r\n");
    isrs_install();

    uart_print(COM1, "Installing IRQs\r\n");
    irq_install();

    irq_install_handler(1, keyboard_handler);
    irq_install_handler(4, uart_handler);
    idt_set_gate(0x80, (unsigned)syscall_handler, 0x08, 0xEE);//int 0x80 by ring3
    
    // timer_install();
    // timer_phase(1);
    //ACPI initialization
    // struct RSDP_t rdsp;
    // rdsp = find_rsdt();//this fella is well above the first 1M so we have to do something about it

    asm volatile("sti"); //enable interrupts

    // //drawing in 800x600
    // uint32_t * framebuffer = (uint32_t *)(mbd->framebuffer_addr_lower);
    // uint32_t height, width;
    // height = mbd->framebuffer_height;
    // width = mbd->framebuffer_width;


    // for(uint32_t i = 0; i < height; ++i){
    //     for(uint32_t j = 0; j < width; ++j){
    //         framebuffer[j + (i * width)] = 0xADAED6;//RGB //nice color
    //     }
    // }
    

    //loading userprogram.bin to memory and possibly execute
    uint32_t * bumpy  = &kernel_phy_end, * bumpy_stack;

    bumpy = align2_4kB(bumpy);
    bumpy_stack = bumpy + 4095;

    typedef union{
        struct{
            uint32_t offset    :12;
            uint32_t table     :10;
            uint32_t directory :10;
        };
        uint32_t virtAddr;
    } virtAddr_t;


    virtAddr_t v = {.virtAddr = (uint32_t)bumpy};

    bootstrap_pde[v.directory] |= 0b00000111; //also user
    uint32_t * pagetable = bootstrap_pde[v.directory] & ~(0xffful);

    pagetable[v.table] &= ~0xfff;
    pagetable[v.table] |= 0b00000111;

    flush_tlb();


    memcpy(bumpy, (void *)modules->mod_start, (uint32_t)(modules->mod_end - modules->mod_start)); //copying user program to memory
    
    asm volatile ("mov %0, %%esi\t\n":: "r"(bumpy):);
    asm volatile ("mov %0, %%edi\t\n":: "r"(bumpy_stack):);
   jump_usermode(bumpy); //problems occurs

    
    for(;;){
    }

}
