#include <vmm.h>



void * vmm_mark_allocated(list_t * map, u32 va, u32 pa, int attributes){

    listnode_t * node = map->head;
    int size = map->size;
    
    // fb_console_printf("\n\n-----------VMAPPING-----------\n");
    for(int i = 0; i < size; ++i){
        
        vmem_map_t * nmap = node->val;
        if( !(nmap->attributes >> 20)  && va >= nmap->vmem && va < nmap->phymem){ //base limit

            vmem_map_t *allocated, *next_free;

            if(nmap->vmem == va){ //at the begining of the memory
                
                next_free = kcalloc(1, sizeof(vmem_map_t));    
                next_free->attributes = (0 << 20) | 0;
                next_free->vmem = va + 0x1000;
                next_free->phymem = nmap->phymem;

                list_insert_end(map, next_free);
                
                allocated = nmap;
                allocated->vmem = va;
                allocated->phymem = pa;
                allocated->attributes = (1 << 20) | attributes;

            }
            else if(nmap->phymem == (va + 0x1000) ){ //at the end of the memory

                nmap->phymem = va;

                allocated = kcalloc(1, sizeof(vmem_map_t));
                allocated->vmem = va;
                allocated->phymem = pa;
                allocated->attributes = (1 << 20) | attributes;
                list_insert_end(map, allocated);


            }
            else{


                allocated = kcalloc(1, sizeof(vmem_map_t));
                next_free = kcalloc(1, sizeof(vmem_map_t));

                //allocated
                allocated->vmem = va;
                allocated->phymem = pa;
                allocated->attributes = (1 << 20) | attributes;
                list_insert_end(map, allocated);
            
                //free
                next_free->vmem = va + 0x1000;
                next_free->phymem = nmap->phymem;
                next_free->attributes = (0 << 20);
                list_insert_end(map, next_free);

                nmap->phymem = va;
            }



            list_sort(map, vmm_compare_function);
            return map;
            // fb_console_printf("--> vmem:%x pmem:%x attributes:%x\n", nmap->vmem, nmap->phymem, nmap->attributes);
            break;
        }


        node = node->next;
    }

    return NULL;
}


void * vmm_get_empty_vpage(list_t * vmap, size_t length){
    if( !(length & ~0xFFFul) ){
        return (void*)-1;
    }

    vmem_map_t * pf;
    listnode_t * node = vmap->head;
    int size = vmap->size;
    for(int i = 0; i < size ; ++i){

        pf = node->val;

        // a free and has enough size
        if( !(pf->attributes >> 20) && length < (pf->phymem - pf->vmem) ){ 

            return (void *)pf->vmem;
        }
        node = node->next;
    }


    return (void*)-1;
}

/*
    well it searches for n_continous free virtual pages in kernel space
    why? well current virtual page allocator get's a free physical page
    from a free physical pages bitmap and maps it to the kernel_heap address
    then increment the kernel heap by a  page, which is obviously not great.
    For instance if i allocate a page through kpalloc, it allocates a physical pages, 
    maps  it to the heap  and heap is incremented and next functions might allocate pages
    thus incrementing the heap so after and if i wan't to free the first page,
    it creates empty section in the mapping so we need to find an 
    empty virtual page with the length of n_continous page
    it returns the virtual address, then caller can map it to a some pyhsical page
    whether it points to the ram or an memory mapped i/o device

    How it works is, we are using recursive paging which page root table is
    mapped to the 1022'th page directory entry so it is accesible by simple addressing.
    since kernel side has all it's directories allocated during the initilization we just
    look in the directories to find empty entries, if we find n_pages then we return the address

    It returns address between 0xc0000000-0xffffffff so if we can't find
    n_continous page entries then it returns NULL
*/
#include <fb.h>
void* vmm_get_empty_kernel_page(int n_continous_page, void* hint_addr){

    //since in kernel side all page directories exist
    //i will treat it like an array of uint32_t[256][1024]
    //which is easier because it's continous

    int found = 0;
    virt_address_t addr = {.address = 0};

    unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * 768); //because it is in kernel memory
    
    //hint address here is used as a base where the search should begin
    //this could be useful because the heap can be extended by well heap allocator
    // which use kpalloc to extend heap
    uint32_t i = 0;
    if(hint_addr < (void*)0xC0000000){
        i = 0;
    }
    else{
        hint_addr = align2_4kB(hint_addr);
        i = (uint32_t)( (uint32_t)hint_addr - 0xC0000000);
        i /= 4096;
    }
    for(; i < 256*1024; ++i){
        
        //let's test it
        if(!(pt[i] & 1)){ //empty entry so
            
            //found the first page 
            if(!addr.address){
                
                addr.directory = 768 + (i/1024);
                addr.table = i % 1024;
            }
            found += 1;
            if(found >= n_continous_page){
                return (void*)addr.address;
            }
        }


    }

    return NULL;
}

int vmm_compare_function(void* val1, void* val2){

    vmem_map_t *addr1 = val1;
    vmem_map_t *addr2 = val2;

    return addr2->vmem < addr1->vmem ? 1: 0;

}