#ifndef __VMM_H__
#define __VMM_H__
#include <sys.h>
#include <g_list.h>

/*
vmm in process control block is a linked list of memory mapping
it can be used for say copying mapping when process is forked,
to find free mappings in virtual address space for mmap
of release pages when process exits
*/

#define VMM_ATTR_EMPTY_SECTION 0
#define VMM_ATTR_PHYSICAL_PAGE 1 << 0
#define VMM_ATTR_PHYSICAL_MMIO 1 << 1
#define VMM_ATTR_PHYSICAL_SHARED 1 << 2

typedef struct{
    u32 vmem;    
    u32 phymem;
    u32 attributes;
    //lower 20bits represent amount of pages unused vpage size if unused
    //upper 12-bits reprenst like they are allocated or not etc my cow or somethin
    
} vmem_map_t;


void * vmm_mark_allocated(list_t * map, u32 va, u32 pa, int attributes);
void * vmm_get_empty_vpage(list_t * vmap, size_t length);



#endif