#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <multiboot.h>
#include <acpi.h>
#include <system.h>
#include <vga.h>
#include <gdt.h>
#include <idt.h>
#include <isrs.h>
#include <irq.h>
#include <timer.h>
#include <kb.h>
#include <uart.h>
#include <k_heap.h>
#include <paging.h>
extern uint32_t kernel_end;
void syscall(){
    printf("Well i'm the motherfuckin syscall. B*tch\n");
    return;
}

int main(multiboot_info_t* mbd, unsigned int magic){
    initVGATerm();

	//multiboot shenanigans
     if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("invalid magic number!");
    }

     if(!(mbd->flags >> 6 & 0x1)) {
        panic("invalid memory map given by GRUB bootloader");
    }
    
    unsigned int i;
    multiboot_memory_map_t* mmmt; //i will pass this around
    for(i = 0; i < mbd->mmap_length;i += sizeof(multiboot_memory_map_t)) 
    {
        mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);
 
        printf("Start Addr: %x | Length: %x | Size: %x | Type: %u\n",
            mmmt->addr_low , mmmt->len_low, mmmt->size, mmmt->type);
        if(mmmt->type == 1){

        }
    }

        


    
    serialInit(COM1, 2 *384000);//max possible /does not work tho
    
    gdt_install();
    idt_install();
    isrs_install();
    idt_set_gate(0x80, (unsigned)syscall,0x8, 0x8E); //syscalll
    irq_install();
    init_paging(); //??
    
    irq_install_handler(1, keyboard_handler);
    irq_install_handler(4, uart_handler);
    sti();
    
    printf("%x", acpiGetRSDPtr());
    //asm volatile("int $0x80");
    //printf("%u : %p", map_page((void *)0xb8000, (void *)0x400000, 3), get_physaddr((void *)0x400000));
    //outportw( 0x604, 0x0 | 0x2000 ); //qemu shutdown
    
    
    
    
   //__asm__  ("div %0" :: "r"(0)); //division by zero exceptoin
    for(;;);

    return 0;
}
