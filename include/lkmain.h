#ifndef _LK_MAIN_H
#define _LK_MAIN_H


#include <multiboot.h>
#include <stdint.h>

extern uint32_t kernel_phy_start[];
extern uint32_t kernel_phy_end[];
extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern void enablePaging(unsigned long int *);
extern void kmain(multiboot_info_t* mbd, unsigned int magic);

void setup_realmode();
void lkmain(multiboot_info_t* mbd, unsigned int magic);









#endif