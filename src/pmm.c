#include <pmm.h>
#include <fb.h>

extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t bootstrap_pte2[1024];

uint32_t * kdir_entry = bootstrap_pde;
uint8_t * bitmap_start = (uint8_t *)&kernel_end;
uint8_t * kernel_heap = (uint8_t *)&kernel_end;;
uint8_t * memstart;
uint32_t bitmap_size;

uint32_t total_block_size;

void pmm_init(uint32_t mem_start, uint32_t mem_size){ //problematic af

    memstart = (void *)mem_start;
    bitmap_size = mem_size / (8 * 4 * 1024); //total blocks as bitss
    
    kernel_heap += bitmap_size;
    kernel_heap = align2_4kB(kernel_heap);
    
    uart_print(COM1, "kernel_heap :%x\r\n", kernel_heap);
    uart_print(COM1, "bitmap_size :%x\r\n", bitmap_size);

    memset(bitmap_start, 0, bitmap_size);
    // kdir_entry = get_physaddr(kdir_entry);
 

}



block_t * mem_list = NULL;	  // All the memory blocks that are


void kmalloc_init(){

    mem_list = (void *)(kernel_heap);
    unmap_virtaddr(kernel_heap);
    map_virtaddr(
        kernel_heap, 
        allocate_physical_page(), 
        PAGE_PRESENT | PAGE_READ_WRITE
    );

    kernel_heap += 0x1000;

    unmap_virtaddr(kernel_heap);
    map_virtaddr(
        kernel_heap, 
        allocate_physical_page(), 
        PAGE_PRESENT | PAGE_READ_WRITE
    );
    kernel_heap += 0x1000;

    //lets say 2 pages
    mem_list->is_free = 1;
    mem_list->size = 4096 * 2;
    mem_list->size -= sizeof(block_t);
    mem_list->prev = NULL;
    mem_list->next = NULL;
    
    return;
}

void * kmalloc(unsigned int size){

    for(block_t * head = mem_list; head != NULL; head = head->next){
        if(head->is_free && size < head->size ){
           block_t  temp;
           block_t * next;
           
           temp = *head;
           next = head;
           
            next->size = size;
            next->is_free = 0;
            next->next = (block_t *)((uint8_t *)(next) + sizeof(block_t) + size);

            next->next->prev = next;
            next = next->next;
            next->size -= size + sizeof(block_t);
            
            return &next->prev[1];

        }
    }
    return NULL;
};

void * kcalloc(u32 nmemb, u32 size){
    void * retval;
    retval = kmalloc(nmemb * size);
    memset(retval, 0, nmemb * size);
    return retval;
}

void * kpalloc(unsigned int npages){ //allocate page aligned pages.
    //for simpilicity npages is assigned to 1;
    npages = 1;

    // return NULL;

    u8 * page = allocate_physical_page();
    if(!page){
        return NULL;
    }

    map_virtaddr(kernel_heap, page, PAGE_PRESENT | PAGE_READ_WRITE);
    page = kernel_heap;
    kernel_heap += 0x1000;

    return page;
    
};


void alloc_print_list(){
    int i = 0;
    for(block_t * head = mem_list; head->next != NULL && head != NULL; head = head->next){
        
        uart_print(COM1, "%u : size->%x\r\n", i, head->size);
    }

}

void kfree(void * ptr){
    block_t * head = &((block_t *)ptr)[-1];
    head->is_free = 1;
    //merge to other free blocks
    if( head->prev->is_free){
        head->prev->next = head->next;
        head->next->prev = head->prev;
        
        head->prev->size += head->size + sizeof(block_t);
    }
    else if(head->next->is_free){
        
    }
    // head->prev->next = head->next;
    // head->next->prev = head->prev;

    //merge free consequent free block to prevent fragmentation
}

void pmm_print_usage(){
    unsigned int used_pages = 0;
    unsigned int free_pages = 0;
    unsigned int start_addr;
    unsigned int end_addr;

    int state = 0;
    for(unsigned int i = 0; i < bitmap_size + 1; ++i){
        for(int bit = 0; bit < 8 ; ++bit){
            int is_found = GET_BIT(bitmap_start[i], bit);

            if(!state){ // no found
                if(is_found){
                    start_addr = (unsigned int)memstart;
                    start_addr += 0x1000*(8*i + bit);
                    state = 1;
                    // bit--;
                }else{
                    free_pages++;
                }
            }
            else{ //found
                if(!is_found){
                    end_addr = (unsigned int)memstart;
                    end_addr = start_addr;
                    end_addr += 0x1000*(8*i + bit);
                    state = 0;
                    uart_print(COM1, "[%x...%x]: Allocated size -> %u\r\n", start_addr, end_addr, end_addr - start_addr);
                }
                else{
                    used_pages++;
                }
            }
        }
    }
    uart_print(COM1, "used_pages = %x, free_pages = %x\r\n", used_pages, free_pages);

}


void * allocate_physical_page(){ //broken af

    for(unsigned int i = 0; i < bitmap_size; ++i){
        for(int bit = 0; bit < 8 ; ++bit){
            if(GET_BIT(bitmap_start[i], bit) == 0){ //empty page
                bitmap_start[i] |= 1 << bit;
                // pmm_print_usage();
                return memstart + (8*i + bit)*4096;
            }
        }
    }
    
    return NULL;
}


int deallocate_physical_page(void * address){ //broken af

    uint32_t addr = (uint32_t)address;
    // uart_print(COM1, "%x marked as free\r\n", address);
    unsigned int bits = addr - (uint32_t)memstart;
    bits /= 4096; //number of bits
    int bit_index = bits % 8; //(8*byte_index + bit_index)
    int byte_index = (bits - bit_index)/8;

    bitmap_start[byte_index] &= ~(1 << bit_index);
    return 0;
}


int pmm_mark_allocated(void * address){
    uint32_t addr = (uint32_t)address;
    // uart_print(COM1, "%x marked as allocated\r\n", address);
    unsigned int bits = addr - (uint32_t)memstart;
    bits /= 4096; //number of bits
    int bit_index = bits % 8; //(8*byte_index + bit_index)
    int byte_index = (bits - bit_index)/8;

    bitmap_start[byte_index] |= (1 << bit_index);
    return 0;
}

void *get_physaddr(void * virtualAddr){
    virt_address_t vf = {.address = (uint32_t)virtualAddr};
    page_directory_entry_t * directory;
    page_table_entry_t * table;

    directory = (void *)bootstrap_pde;
    if(!directory[vf.directory].present) return NULL;
    
    table = (void *) ( (directory[vf.directory].raw& ~0xFFF) + 0xc0000000 );
    

    if(!table[vf.table].present) return NULL;

    return (void *)( vf.offset + (table[vf.table].raw & ~0xFFF) );

}

int is_virtaddr_mapped(void * virtaddr){
    virt_address_t v;
    page_directory_entry_t * dir = (void *)bootstrap_pde;
    page_table_entry_t * table;
    v.address = (u32)virtaddr;
    //first check if that table is mapped
    if( !(dir[v.directory].raw & 1) ){ //not present
        return 0;
    }

    //if table exist then check in the table if corresponding entry exists
    table = (void *) ( (dir[v.directory].raw& ~0xFFF) + 0xc0000000 );

    if(!(table[v.table].raw & 1)){
        return 0;

    }

    //well it should exist
    return 1;
}

int is_virtaddr_mapped_d(void * _dir, void * virtaddr){
    virt_address_t v;
    page_directory_entry_t * dir = _dir;
    page_table_entry_t * table;
    v.address = (u32)virtaddr;
    //first check if that table is mapped
    if( !(dir[v.directory].raw & 1) ){ //not present
        return 0;
    }

    //if table exist then check in the table if corresponding entry exists
    table = (void *) ( (dir[v.directory].raw& ~0xFFF) + 0xc0000000 );

    if(!(table[v.table].raw & 1)){
        return 0;

    }

    //well it should exist
    return 1;
}


void map_virtaddr(void * virtual_addr, void * physical_addr, uint16_t flags){
    virt_address_t v, p;
    page_directory_entry_t * directory = (void*)bootstrap_pde;
    page_table_entry_t * table;

    v.address = (uint32_t)virtual_addr;
    p.address = (uint32_t)physical_addr;

    directory[v.directory].raw |= flags;
    //raw address so add 0xc0000000 to acces it without causing page fault
    table = (void*)((directory[v.directory].raw & ~(0xffful)) + 0xc0000000);

    //check if table is mapped or not mapped to the kernel heap then unmap it
 
    
    
    table[v.table].raw = (uint32_t)p.address;
    table[v.table].raw &= ~0xFFF;
    table[v.table].raw |=  0b111;

    
    return;

}

void unmap_virtaddr(void * virtual_addr){
    virt_address_t v;
    page_directory_entry_t * directory = (void*)bootstrap_pde;
    page_table_entry_t * table;

    v.address = (uint32_t)virtual_addr;
    table = (void*)(directory[v.directory].raw & ~(0xffful));

    //directory[v.directory].raw = 0; //not necessarily
    table[v.table].raw = 0;
    
    return;
    
}


void *get_physaddr_d( void * _directory, void * virtualAddr){
    virt_address_t vf = {.address = (uint32_t)virtualAddr};
    page_directory_entry_t * directory;
    page_table_entry_t * table;

    directory = _directory;
    if(!directory[vf.directory].present) return NULL;
    
    table = (void *) (directory[vf.directory].raw& ~0xFFF);
    if(!table[vf.table].present) return NULL;

    return (void *)( vf.offset + (table[vf.table].raw & ~0xFFF) );

}

void map_virtaddr_d(void * _directory, void * virtual_addr, void * physical_addr, uint16_t flags){
    virt_address_t v, p;
    page_directory_entry_t * directory = (void*)_directory;
    page_table_entry_t * table;

    v.address = (uint32_t)virtual_addr;
    p.address = (uint32_t)physical_addr;

    //check if directory has table
    if(!directory[v.directory].present){ //yok yeni sayfa al
        page_table_entry_t * t = kpalloc(1);
        memset(t, 0, 4096);
        t[v.table].raw = (uint32_t)p.address;
        t[v.table].raw &= ~0xFFF;
        t[v.table].raw |=  0b111;

        directory[v.directory].raw = (uint32_t)get_physaddr(t);
        directory[v.directory].raw |= flags;

    }
    else{
        directory[v.directory].raw |= flags;
        table = (void*)((directory[v.directory].raw & ~(0xffful)));
        table[v.table].raw = (uint32_t)p.address;
        table[v.table].raw &= ~0xFFF;
        table[v.table].raw |=  0b111;
    }



    

    return;

}

void unmap_virtaddr_d( void * _directory, void * virtual_addr){
    virt_address_t v;
    page_directory_entry_t * directory = (void*)_directory;
    page_table_entry_t * table;

    v.address = (uint32_t)virtual_addr;
    table = (void*)(directory[v.directory].raw & ~(0xffful));

    //directory[v.directory].raw = 0; //not necessarily
    table[v.table].raw = 0;
    
    return;
    
}



void unmap_everything(){
    
    memset(bootstrap_pde, 0, 1024*4);
    memset(bootstrap_pte1, 0, 1024*4);
    memset(bootstrap_pte2, 0, 1024*4);
    return;
}