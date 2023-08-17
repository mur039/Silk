#include <alloc.h>
extern uint32_t kernel_end;
uint32_t * placement_address = &kernel_end;

void * kmalloc(uint32_t size){
    uint32_t *  mem =  placement_address;
	placement_address += size;
	return mem;
}

    

