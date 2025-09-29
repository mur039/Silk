#include <vmm.h>
#include <pmm.h>


void * vmm_get_empty_vpage(struct mm_struct* mm, uintptr_t hint, size_t length){
    
    if( !(length & ~0xFFFul) ){
        return (void*)-1;
    }

    uintptr_t start = hint ? (uintptr_t)align2_4kB(hint) : VMM_USERMEM_START;
    for(struct vma* head = mm->mmap, *prev = NULL; head ; head = head->next){

        if(start + length <= head->start){
            if(!prev || start >= prev->end){
                return (void*)start;
            }
        }

        if(start < head->end){
            start = (uintptr_t)align2_4kB(head->end);
        }

        prev = head;
    }

    //well shit check for the end of it too
    if(start + length <= VMM_USERMEM_END){
        return (void*)start;
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

//called from isr-pagefault
#include <syscalls.h>
int vmm_handle_pagefault(struct mm_struct *mm, uintptr_t addr, uint32_t err_code){
    
    if(addr < VMM_USERMEM_START || addr >= VMM_USERMEM_END && (err_code & PAGE_FAULT_US)){ //if userspace and out of userspace range
        return -EINVAL;
    }

    struct vma* segment = vmm_find_vma(mm, addr);
    if(!segment){
        return -EINVAL;
    }
    
    
    if(segment->flags & VM_GROWSDOWN){
        uint8_t* page = allocate_physical_page();
        if(!page){
            return 0;
        }
        map_virtaddr((void*)addr,page, PAGE_READ_WRITE | PAGE_USER_SUPERVISOR | PAGE_PRESENT);
        return 1;
    }

    struct vm_operations* ops = segment->ops;
    if(ops && ops->fault){
        return ops->fault(segment, addr, err_code);
    }

    return 0;
}

int vmm_insert_vma(struct mm_struct* mm, struct vma* v){
    struct vma **pp = &mm->mmap;
    while (*pp && (*pp)->start < v->start) {
        pp = &(*pp)->next;
    }
    v->next = *pp;
    *pp = v;

    return 0;
}

struct vma* vmm_find_vma(struct mm_struct* mm, uintptr_t addr){

    for(struct vma* head = mm->mmap; head ; head = head->next){
        if(addr >= head->start && addr < head->end){
            return head;
        }
    }
}
