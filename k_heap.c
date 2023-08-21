#include <k_heap.h>
extern uint32_t kernel_end;
static uint32_t placement_address = (uint32_t)&kernel_end;

uint32_t kmalloc(uint32_t size){
    uint32_t  mem =  placement_address;
	placement_address += size;
	return mem;
}

uint32_t kmalloc_page(){
	if (placement_address & 0xFFFFF000) { // check if shit is aligned or not
        // Align the placement address;
		placement_address &= 0xFFFFF000;
		placement_address += 0x1000;
    	}
	return kmalloc(0x1000);
}


