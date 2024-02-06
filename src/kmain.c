#include <multiboot.h>
#include <stdint.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <uart.h>
#include <acpi.h>
#include <mm.h>
#include <string.h>
extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];

void syscall_handler(struct regs *r){
    for(;;);
}

void keyboard_handler(struct regs *r){
    for(;;);
}
uint32_t kernel_stack[2048]; //8kB stack

 void kmain(multiboot_info_t* mbd){ //high kernel
    
    asm volatile("cli");
    init_uart_port(0x3F8); //COM1
    uart_write(0x3F8, "Merhabalar Efenim\r\n", 1, 20);

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
        uart_write(0x3F8, buffer, 1, 64);
           
    }

    // uart_write(0x3F8, "Installing Interrupt Descriptor Tables\r\n", 1, 41);
    // idt_install();

    // uart_write(0x3F8, "Installing ISRs\r\n", 1, 18);
    // isrs_install();

    // uart_write(0x3F8, "Installing IRQs\r\n", 1, 18);
    // irq_install();
    // irq_install_handler(1, keyboard_handler);
    // irq_install_handler(4, uart_handler);
    // irq_install_handler(3, uart_handler);
    // idt_set_gate(0x80, (unsigned)syscall_handler, 0x08, 0x8E);
    // idt_load();
    //asm volatile("int $0x80"); //syscall
    //ACPI initialization
    // struct RSDP_t rdsp;
    // rdsp = find_rsdt();//this fella is well above the first 1M so we have to do something about it


    //drawing in 800x600
    uint32_t * framebuffer = (uint32_t *)(mbd->framebuffer_addr_lower);
    uint32_t height, width;
    height = mbd->framebuffer_height;
    width = mbd->framebuffer_width;

    for(uint32_t i = 0; i < height; ++i){
        for(uint32_t j = 0; j < width; ++j){
            framebuffer[j + (i * width)] = 0xADAED6;//RGB //nice color
        }
    }
    
    for(;;);
    return;
}
