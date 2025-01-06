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

