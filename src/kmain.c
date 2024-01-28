#include <multiboot.h>
#include <stdint.h>
extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
    
 void kmain(multiboot_info_t* mbd, unsigned int magic){ //high kernel



    
    uint32_t * framebuffer = (uint32_t *)(mbd->framebuffer_addr & 0xFFFFFFFF);
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
