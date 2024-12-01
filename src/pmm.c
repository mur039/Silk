#include <pmm.h>
#include <fb.h>

extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t bootstrap_pte2[1024];
extern uint32_t kernel_phy_start;

uint32_t * kdir_entry = bootstrap_pde;
uint8_t * bitmap_start = (uint8_t *)&kernel_end;
uint8_t * kernel_heap = (uint8_t *)&kernel_end;;
uint8_t * memstart;
uint32_t bitmap_size;

uint32_t * memory_window = (uint32_t *)(0xC0100000 - 0x1000);

uint32_t total_block_size;

uint32_t * current_page_dir;






void pmm_init(uint32_t mem_start, uint32_t mem_size){ //problematic af

    memstart = (void *)mem_start;
    bitmap_size = mem_size / (8 * 4 * 1024); //total blocks as bitss
    
    kernel_heap += bitmap_size;
    kernel_heap = align2_4kB(kernel_heap);
    
    uart_print(COM1, "kernel_heap :%x\r\n", kernel_heap);
    uart_print(COM1, "bitmap_size :%x\r\n", bitmap_size);

    memset(bitmap_start, 0, bitmap_size);
    // kdir_entry = get_physaddr(kdir_entry);
    current_page_dir = kdir_entry;
 

}



block_t * mem_list = NULL;	  // All the memory blocks that are


void kmalloc_init(int npages){

    if(npages < 0){
        //punish him
        halt();
    }
    
    mem_list = (void *)(kernel_heap);

    for(int i = 0; i < npages ; ++i){
        
        unmap_virtaddr(kernel_heap);
        map_virtaddr(
            kernel_heap, 
            allocate_physical_page(), 
            PAGE_PRESENT | PAGE_READ_WRITE
        );

        kernel_heap += 0x1000;
    }

    // unmap_virtaddr(kernel_heap);
    // map_virtaddr(
    //     kernel_heap, 
    //     allocate_physical_page(), 
    //     PAGE_PRESENT | PAGE_READ_WRITE
    // );
    // kernel_heap += 0x1000;

    //lets say 2 pages
    mem_list->is_free = 1;
    mem_list->size = 4096 * npages;
    mem_list->size -= sizeof(block_t);
    mem_list->prev = NULL;
    mem_list->next = NULL;
    
    return;
}

void * kmalloc(unsigned int size){

    #if DEBUG
    alloc_print_list();
    #endif

    for(block_t * head = mem_list; head != NULL; head = head->next){
        if(head->is_free && size < head->size ){


           block_t  temp;
           block_t  * next;
           
           temp = *head;

            next = head;
            next->size = size;
            next->is_free = 0;
            next->next = (block_t *)((uint8_t *)(next) + sizeof(block_t) + size);

            
            next->next->prev = next;
            next = next->next;
            next->size = temp.size - (size + sizeof(block_t));
            next->next = NULL;
            next->is_free = 1;
            


            return &next->prev[1];

        }
    }
    return NULL;
};

void * krealloc( void *ptr, size_t size){
    
    void * retval = ptr;
    block_t * metadata = (block_t *)ptr;
    metadata = &metadata[-1];

   if(size > metadata->size){
      void * new = kmalloc(size);
      memcpy(new, &metadata[1], metadata->size);
      retval = new;
      kfree(ptr);
   }
    
    return retval;
}

void * kcalloc(u32 nmemb, u32 size){
    void * retval;
    retval = kmalloc(nmemb * size);
    memset(retval, 0, nmemb * size);
    return retval;
}


void * kpalloc(unsigned int npages){ //allocate page aligned pages.
    //for simpilicity npages is assigned to 1;
    // npages = 1;

    // return NULL;

    u8 * ret = NULL;

    for(unsigned int i = 0; i < npages; ++i){

    u8 * page = allocate_physical_page();
    if(!page){
        return NULL;
    }

    map_virtaddr(kernel_heap, page, PAGE_PRESENT | PAGE_READ_WRITE);
    page = kernel_heap;

    if(!ret) ret = page;

    kernel_heap += 0x1000; // does it need tho?

    }
    
/*
    since this address is 4kB aligned lowest 12-bits are empty i can cram some information here too
    or maybe not wecause when page is given to a process this info will be lost as well :(
    well since these pages are mapped on to the heap for n = 1
    i can just unmap it but for n!= 1 i need to know the amount i allocated

    
*/
    return ret;
    
};

void kpfree(void * address){
    void * phy = get_physaddr(address);
    unmap_virtaddr(address);
    deallocate_physical_page(phy);
}


void alloc_print_list(){
    uart_print(COM1, "*alloc list\r\n");
    int i = 0;
    for(block_t * head = mem_list; /*head->next != NULL &&*/ head != NULL; head = head->next){
        
        uart_print(COM1, "\t%u : size->%x, isFree->%x\r\n", i, head->size, head->is_free);
        i++;
    }

}

void kfree(void * ptr){
    // uart_print(COM1, "KFREE TODO\r\n");
    block_t * head = (block_t *)ptr;
    head -= 1;
    head->is_free = 1;

    // //merge to other free blocks
    if( head->prev->size > 0){
        head->prev->next = head->next;
        head->next->prev = head->prev;
        
        head->prev->size += head->size + sizeof(block_t);
    }
    else if(head->next->size > 0){
        
    }
    head->prev->next = head->next;
    head->next->prev = head->prev;

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

    directory = (void *)current_page_dir;
    if(!directory[vf.directory].present) return NULL;
    

    //you see

    table = (void *) ( (directory[vf.directory].raw& ~0xFFF));
    map_virtaddr(memory_window, table, PAGE_PRESENT | PAGE_READ_WRITE);
    table = memory_window;

    if(!table[vf.table].present) return NULL;

    return (void *)( vf.offset + (table[vf.table].raw & ~0xFFF) );

}

int is_virtaddr_mapped(void * virtaddr){
    virt_address_t v;
    page_directory_entry_t * dir = (void *)current_page_dir;
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
    page_directory_entry_t * directory = (void*)current_page_dir;
    page_table_entry_t * table;

    v.address = (uint32_t)virtual_addr;
    p.address = (uint32_t)physical_addr;

    if(!directory[v.directory].present){ //table doesn't exist so allocate and map one?
        u8 * page = kpalloc(1); //maybe it will work? for sometime?
        if(!page) return; //failed to allocate?
        memset(page, 0, 4096);
        directory[v.directory].raw = (uint32_t)get_physaddr(page);
        directory[v.directory].raw |= flags;
        table = (page_table_entry_t*)page;



    }
    else{
    directory[v.directory].raw |= flags;
    table = (void*)((directory[v.directory].raw & ~(0xffful)) + 1* 0xc0000000);

    // map_virtaddr(memory_window, table, PAGE_PRESENT | PAGE_READ_WRITE);
    // table = memory_window;

    }


    //check if table is mapped or not mapped to the kernel heap then unmap it
    
    
    table[v.table].raw = (uint32_t)p.address;
    table[v.table].raw &= ~0xFFF;
    table[v.table].raw |=  0b111;
    asm("invlpg (%0)" : :  "r"(virtual_addr));
    // flush_tlb();    
    
    return;

}

void unmap_virtaddr(void * virtual_addr){
    virt_address_t v;
    page_directory_entry_t * directory = (void*)current_page_dir;
    page_table_entry_t * table;

    v.address = (uint32_t)virtual_addr;
    table = (void*)(directory[v.directory].raw & ~(0xffful));
    
    map_virtaddr(memory_window, table, PAGE_PRESENT | PAGE_READ_WRITE);
    table = (void*)memory_window;
    
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

        // for(int i = 0; i < 4096 ; ++i){
        //     char * phead = (void*)t;
        //     uart_print(COM1, "%x ", phead[i]);
        //     if( i && !(i%16)) uart_print(COM1, "\r\n");
        // }

        directory[v.directory].raw = (uint32_t)get_physaddr(t);
        
        kernel_heap -= 0x1000;
        unmap_virtaddr_d(directory, kernel_heap);

        directory[v.directory].raw |= flags;
        // uart_print(COM1, "directory[v.directory]: %x\r\n", directory[v.directory].raw);

        // uart_print(COM1, "a page is allocated for table\r\n");
        // paging_directory_list(_directory); halt();

        t[v.table].raw = (uint32_t)p.address;
        t[v.table].raw &= ~0xFFF;
        t[v.table].raw |=  flags;


    }
    else{
        directory[v.directory].raw |= flags;
    
        table = (void*)((directory[v.directory].raw & ~(0xffful)));
        map_virtaddr(memory_window, table, PAGE_PRESENT | PAGE_READ_WRITE); //only read darling
        table = (page_table_entry_t*)memory_window;

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
    
    map_virtaddr(memory_window, table, PAGE_PRESENT | PAGE_READ_WRITE);
    table = (page_table_entry_t *)memory_window;

    table[v.table].raw = 0;
    
    return;
    
}



void unmap_everything(){
    
    memset(bootstrap_pde, 0, 1024*4);
    memset(bootstrap_pte1, 0, 1024*4);
    memset(bootstrap_pte2, 0, 1024*4);
    return;
}

#include <process.h>
#include <g_list.h>

void paging_directory_list(void * dir_entry){
    pcb_t * process;
    
    
    /*
    for physical addresses  we use last entry in the kernel table as temporary page where it will act 
    like a window into the memory
    */

    page_directory_entry_t * process_page_dir = dir_entry ;//current_process->page_dir;
    page_table_entry_t * process_page_table;

    //its in the kernel so probably it is mapped
    virt_address_t va = {.address = 0};
    
    

    for(int dir = 0; dir < 768; dir++){
        if(process_page_dir[dir].present){
            
            void * table_phy_addr = (void *)( process_page_dir[dir].raw & ~0xfff);
            
           
            /*
            since it is physical memory address and there's no way i can find where its mapped
            in virtual memory i will map it in the page right below the kernel_start
            Since this mapping will be in directory copied from kernel's own directory i can 
            use map_virtaddr
            */
            // unmap_virtaddr(memory_window);
            map_virtaddr(memory_window, table_phy_addr, PAGE_PRESENT); //only read darling
            
            
            process_page_table = (page_table_entry_t*)memory_window;

            for(int table = 0; table < 1024; table++){

                if(process_page_table[table].present){
                    void * phy_addr = (void *)(process_page_table[table].raw & ~0xfff); 


                    va.address = 0; va.directory = dir; va.table = table;
                    fb_console_printf(
                        "-> %x : %x -> dir:%u table:%u  flags:%x %x\n",
                        va.address, 
                        phy_addr,
                        dir,
                        table,
                        process_page_dir[dir].raw & 0xffful,
                        (process_page_table[table].raw & 0xffful)
                        );
                }
            }    
        }


    }


}

