#ifndef __VMM_H__
#define __VMM_H__
#include <sys.h>
#include <g_list.h>
#include <stddef.h>
#include <stdint-gcc.h>


#define VMM_USERMEM_START 0x10000
#define VMM_USERMEM_END 0xc0000000

//well fuck we go linux way...
#define VM_READ   0x1
#define VM_WRITE  0x2
#define VM_EXEC   0x4
#define VM_SHARED 0x8   // MAP_SHARED
#define VM_PRIVATE 0x10 // MAP_PRIVATE
#define VM_STACK  0x20
#define VM_GROWSDOWN 0x40


struct vma;
struct vm_operations {
    void (*open)(struct vma *vma);
    void (*close)(struct vma *vma);
    // fault: returns 0 on success, -ERR on error
    int  (*fault)(struct vma *vma, uintptr_t fault_addr, uint32_t err);
    // optional: page is being evicted / put
    // void (*page_free)(struct vma *vma, struct page *p);
};

struct vma {
    uintptr_t start;        // inclusive
    uintptr_t end;          // exclusive
    unsigned long flags;    // VM_READ|VM_WRITE|...
    size_t file_offset;      // file offset if file-backed
    struct fs_node *file;   // NULL if anonymous
    struct vm_operations *ops;
    struct vma *next;       // simple list or RB-tree pointers
};

struct mm_struct {
    struct vma *mmap;       // head of vma list (sorted by start)
    //a spinlock would be good actually
};


int vmm_handle_pagefault(struct mm_struct *mm, uintptr_t addr, uint32_t err_code);
int vmm_insert_vma(struct mm_struct* mm, struct vma* v);
struct vma* vmm_find_vma(struct mm_struct* mm, uintptr_t addr);

void * vmm_get_empty_vpage(struct mm_struct* mm, uintptr_t hint, size_t length);
void* vmm_get_empty_kernel_page(int n_continous_page, void* hint_addr);

#endif