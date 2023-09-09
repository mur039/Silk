#include <multiboot.h>
#include <stdint.h>
extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
    
 void kmain(multiboot_info_t* mbd, unsigned int magic){
    short (* HvideoBuffer)[80] = (short (*)[80])0xC00B8000;
    short (* LvideoBuffer)[80] = (short (*)[80])0x000B8000;

    //bootstrap_pde[0] ^= 1UL;

    HvideoBuffer[0][0] = 0xF0 << 8 | 'A';
    LvideoBuffer[0][1] = 0xF0 << 8 | 'B';


    for(;;);
    return;
}
