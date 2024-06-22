#include <pmm.h>
extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t bootstrap_pte2[1024];

uint32_t * K_PDE = bootstrap_pde;
uint8_t * bitmap_start = (uint8_t *)&kernel_end;
uint32_t total_block_size;
uint8_t * memstart;

void pmm_init(uint32_t memsize){ //problematic af
    return;
    total_block_size = memsize / (BLOCK_SIZE * 8); //byte exactly
    //allocating page for that
    for(unsigned int  i = 0; i < (total_block_size >> 12); ++i){
        bootstrap_pte1[i + ((uint32_t)bitmap_start >> 12)] = ((i * 0x1000) + ((uint32_t)(bitmap_start - 0xC0000000) >> 12)) | 3;
        
    }
    flush_tlb();
    memset(bitmap_start, 0, total_block_size);
    memstart = (uint8_t *)(bitmap_start + total_block_size);
    return;
}


void *get_physaddr(void * virtualAddr){
    union virtAddr_f vf = {.virtAddr = (uint32_t)virtualAddr};
    // //only look for whether its present or not
    if( !(bootstrap_pde[vf.directory] & 1) ) return NULL;
    uint32_t * pagetable = (uint32_t *)(bootstrap_pde[vf.directory] & ~0x3FF);
    if( !(pagetable[vf.table] & 1) ) return NULL;

    // if( (bootstrap_pde[vf.directory] & 1) && (((uint32_t *)bootstrap_pde[vf.directory])[vf.table] & 1)){
    return (void *)((uint32_t)vf.offset + ( pagetable[vf.table] & 0xFFFFF000));
    // }
}


// void *vmm_get_physaddr(void *virtualaddr) {
//     unsigned long pdindex = (unsigned long)virtualaddr >> 22;
//     unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;
    
//     // unsigned long *pd = (unsigned long *)0xFFFFF000;
//     if(!(bootstrap_pde[pdindex] & 1)) return NULL;
    
 
//     unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
//     // Here you need to check whether the PT entry is present.
 
//     return (void *)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
// }
