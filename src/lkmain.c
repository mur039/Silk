#include <lkmain.h>


void lkmain(multiboot_info_t* mbd, unsigned int magic){

    uint32_t * PDE =  (uint32_t *)((uint32_t)bootstrap_pde - 0xC0000000);
    uint32_t * PTE1 = (uint32_t *)((uint32_t)bootstrap_pte1 - 0xC0000000);

    PDE[0] = (uint32_t)PTE1 | 0b00000011;
    PDE[768] = (uint32_t)PTE1 | 0b00000011;

    for(unsigned int i = 0; i < ((uint32_t)kernel_phy_end >> 12); ++i){
        PTE1[i] = (i * 0x1000) | 3;
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

struct gdt_entry gdt[3]; //NULL COde DATA
struct gdt_ptr gp;

void setup_realmode(){
    extern uint8_t rmode_start[];
    extern uint8_t rmode_end[];
    
    uint32_t rmode_size = (uint32_t)(rmode_end - rmode_start);
    uint8_t * readmode_head = (uint8_t *)0x7C00;
    for(uint32_t i = 0; i < (uint32_t)rmode_size;   ++i){
        readmode_head[i] = rmode_start[i];
    }
    
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (unsigned int)&gdt; //wut

    //NULL
    gdt[0].base_low = 0;
    gdt[0].base_middle = 0;
    gdt[0].base_high = 0;
    gdt[0].limit_low = 0;
    gdt[0].granularity = 0;
    gdt[0].access = 0;

    //TEXT
    gdt[1].base_low = 0;
    gdt[1].base_middle = 0;
    gdt[1].base_high = 0;
    gdt[1].limit_low = 0xFFFF;
    gdt[1].granularity = 0x0F;
    gdt[1].access = 0x9A;
    
    //DATA
    gdt[2].base_low = 0;
    gdt[2].base_middle = 0;
    gdt[2].base_high = 0;
    gdt[2].limit_low = 0xFFFF;
    gdt[2].granularity = 0x0F;
    gdt[2].access = 0x92;
    
    

    return;
}



