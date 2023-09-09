
#include <multiboot.h>
#include <stdint.h>
extern uint32_t kernel_phy_start[];
extern uint32_t kernel_phy_end[];
extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern void enablePaging(unsigned long int *);
extern void kmain(multiboot_info_t* mbd, unsigned int magic);

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