#include <lkmain.h>


void lkmain(multiboot_info_t* mbd, unsigned int magic){

    uint32_t * PDE =  (uint32_t *)((uint32_t)bootstrap_pde - 0xC0000000);
    uint32_t * PTE1 = (uint32_t *)((uint32_t)bootstrap_pte1 - 0xC0000000);
    uint32_t * PTE2 = (uint32_t *)((uint32_t)bootstrap_pte2 - 0xC0000000);
    PDE[0] = (uint32_t)PTE1 | 0b00000011;
    PDE[768] = (uint32_t)PTE1 | 0b00000011;
    PDE[1012] = (uint32_t)PTE2 | 0b00000011;

    for(unsigned int i = 0; i < ((uint32_t)kernel_phy_end >> 12); ++i){
        PTE1[i] = (i * 0x1000) | 3;
    }

    for(unsigned int i = 0; i < 1024; ++i){
        PTE2[i] = (0xFD000000 + (i * 0x1000)) | 3;
    }
    enablePaging(PDE);
    kmain(mbd, magic);
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
