#include <lkmain.h>
extern uint32_t kernel_stack[2048];
extern void _changeSP(uint32_t * nsp, int (* foo)(multiboot_info_t * mbd), multiboot_info_t * mbd);

void lkmain(multiboot_info_t* mbd, unsigned int magic){
    
    if(magic != 0x2BADB002) {
	return; //to the halt loop
    }

    //initial paging
    //these are in the high memory range
    uint32_t * PDE =  (uint32_t *)((uint32_t)bootstrap_pde - 0xC0000000);  
    uint32_t * PTE1 = (uint32_t *)((uint32_t)bootstrap_pte1 - 0xC0000000);
    uint32_t * PTE2 = (uint32_t *)((uint32_t)bootstrap_pte2 - 0xC0000000);


    PDE[0]    = (uint32_t)PTE1 | 0b00000011;
    PDE[768]  = (uint32_t)PTE1 | 0b00000011;

    for(unsigned int i = 0; i <  1024; i++){  //whole 4MB
        PTE1[i] = (i * 0x1000) | 3; 
    }

    
    //recursive paging 
    PDE[1023] = (uint32_t)PDE | 3;
    enablePaging(PDE);
    _changeSP(kernel_stack, (int (*)(multiboot_info_t * mbd))kmain  , mbd);
    return;
}

//ah fine
struct gdt_entry
{
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));

/* Special pointer which includes the limit: The max bytes
*  taken up by the GDT, minus 1. Again, this NEEDS to be packed */
struct gdt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));
