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
}


void *get_physaddr(void * virtualAddr){
    virt_address_t vf = {.address = (uint32_t)virtualAddr};
    page_directory_entry_t * directory;
    page_table_entry_t * table;

    directory = (void *)bootstrap_pde;
    if(!directory[vf.directory].present) return NULL;
    
    table = (void *) (directory[vf.directory].raw& ~0xFFF);
    if(!table[vf.table].present) return NULL;

    return (void *)( vf.offset + (table[vf.table].raw & ~0xFFF) );

}

void map_virtaddr(void * virtual_addr, void * physical_addr, uint16_t flags){
    virt_address_t v, p;
    page_directory_entry_t * directory = (void*)bootstrap_pde;
    page_table_entry_t * table;

    v.address = (uint32_t)virtual_addr;
    p.address = (uint32_t)physical_addr;

    directory[v.directory].raw |= flags;
    table = (void*)(directory[v.directory].raw & ~(0xffful));

    table[v.table].raw = (uint32_t)p.address;
    table[v.table].raw &= ~0xFFF;
    table[v.table].raw |=  0b111;
    return;

}
