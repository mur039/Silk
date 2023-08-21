#include <paging.h>
#include <vga.h>
extern void enablePaging(unsigned int*);

// paging mode : 
// NON PAE 4K Mode (32 bit) - level 0 page, 1 PT, 2 PD 
// NON PAE 4M Mode (32 bit) - level 0 page, 2 PD 
// PAE 4K Mode (32 bit) - level 0 page, 1 PT, 2 PD, 3 PDP 
// PAE 4M Mode (32 bit) - level 0 page, 2 PD, 3 PDP
// LONG MODE 4K (64 bit) - level 0 page, 1 PT, 2 PD, 3 PDP, 4 PML4


unsigned int page_directory[NUM_PAGES] __attribute__((aligned(PAGE_FRAME_SIZE)));
unsigned int page_table[NUM_PAGES] __attribute__((aligned(PAGE_FRAME_SIZE)));
union virtAddr_f{
    struct{
        uint32_t offset    :12;
        uint32_t table     :10;
        uint32_t directory :10;
    };
    uint32_t virtAddr;
};

void init_paging() {
	int i;
	// Create page directory, supervisor mode, read/write, not present : 0 1 0 = 2   
	for (i = 0; i < NUM_PAGES; i++) {
		page_directory[i] = 0x00000002; //every page is present aaa
        //page_directory[i] = (unsigned int)directory;    
     	}     

	// Create page table, supervisor mode, read/write, present : 0 1 1 = 3   
	// As the address is page aligned, it will always leave 12 bits zeroed.  
	for (i = 0; i < NUM_PAGES; i++) { 
	        page_table[i] = ((i) * 0x1000) | 3;
	}	

	// put page_table into page_directory supervisor level, read/write, present
	page_directory[0] = ((unsigned int)page_table) | 0x3; //first 4M? //identity maping kernel 
    //page_directory[768] = ((unsigned int)page_table) | 0x3; //
   	enablePaging(page_directory);

   	//register_interrupt_handler(14, handle_page_fault);
}

void page_fault() {
    printf("Page fault asko\n");
	return;
}

void *get_physaddr(void * virtualAddr){
    union virtAddr_f vf = {.virtAddr = (uint32_t)virtualAddr};
    //only look for whether its present or not
    if( (page_directory[vf.directory] & 1) && (page_table[vf.table] & 1)){
        return (void *)((uint32_t)vf.offset + (page_table[vf.table] & 0xFFFFF000));
    }
    return NULL;
}

int map_page(void *physaddr, void *virtualaddr, unsigned int flags){
    union virtAddr_f vf ={.virtAddr = (uint32_t)virtualaddr};
    if( (page_directory[vf.directory] & 1) && (page_table[vf.table] & 1)){
        page_table[vf.table] = ((uint32_t)physaddr & 0xFFFFF000) | flags;
        asm volatile("mov %cr3, %eax\n\tmov %eax, %cr3");
        return 0;
    }
    return -1;
}
