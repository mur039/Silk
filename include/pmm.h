#ifndef __PMM_H_
#define __PMM_H_
#include <sys.h>
#include <stdint.h>
#include <str.h>

/*only works after high-kernel initialized*/
#define BLOCK_SIZE 4096
extern uint32_t kernel_end;
extern uint32_t kernel_phy_end;
extern uint32_t * memory_window;

typedef union {
    struct{
        uint32_t offset    :12;
        uint32_t table     :10;
        uint32_t directory :10;
    };
    uint32_t address;
} virt_address_t;

/*
Virtual Address = 
0xFFC00000 + (PDE_Index * 0x1000) + (PTE_Index * 4)

*/
#define RECURSIVE_PD_INDEX 1023

#define PAGE_PRESENT         0b00000001    
#define PAGE_READ_WRITE      0b00000010    
#define PAGE_USER_SUPERVISOR 0b00000100            
#define PAGE_WRITE_THROUGH   0b00001000        
#define PAGE_CACHE_DISABLED  0b00010000        
#define PAGE_ACCESED         0b00100000    
#define PAGE_DIRTY           0b01000000

#define PAGE_PAGE_SIZE       0b10000000    
#define PAGE_GLOBAL          0b10000000    



#define PAGE_NOT_MAPPED (void*)-1

typedef union{
    struct{
        uint32_t present: 1 ;
        uint32_t read_write:1;
        uint32_t user_supervisor:1;
        uint32_t write_through:1;
        uint32_t cache_disabled:1;
        uint32_t accesed:1;
        uint32_t dirty:1;
        uint32_t page_size:1;
        uint32_t available:3;

        uint32_t :21;

    } __attribute__((packed));
    uint32_t raw;
} page_directory_entry_t;

typedef union{
    struct{
        uint32_t present: 1 ;
        uint32_t read_write:1;
        uint32_t user_supervisor:1;
        uint32_t write_through:1;
        uint32_t cache_disabled:1;
        uint32_t accesed:1;
        uint32_t dirty:1;
        uint32_t global:1;
        uint32_t available:3;

        uint32_t:21;

    } __attribute__((packed));
    uint32_t raw;
} page_table_entry_t;

extern uint32_t * kdir_entry;
extern uint8_t * kernel_heap;
extern uint32_t * current_page_dir;

void map_virtaddr(void * virtual_addr, void * physical_addr, uint16_t flags);
void unmap_virtaddr(void * virtual_addr);
int set_virtaddr_flags(void * virtualaddr, uint16_t flags);
int is_virtaddr_mapped(void * virtaddr);
int is_virtaddr_mapped_d(void * _dir, void * virtaddr);


void *get_physaddr(void * virtualAddr);
void unmap_everything();
void pmm_init(uint32_t mem_start, uint32_t mem_size);
void pmm_print_usage();
int pmm_mark_allocated(void * address);

void * allocate_physical_page();
int deallocate_physical_page(void * address);



#define KMALLOC_MAGIC 0xCAFEBABE
#define KMALLOC_FREE 1
#define KMALLOC_ALLOCATED 0

struct Block
{   
    //it is used as a boolean after all so why not use the rest as magic
    unsigned int is_free;
    unsigned int size;
    
    struct Block * prev;
    struct Block * next;
};

typedef struct Block block_t;
void kmalloc_init(int npages);
void * kmalloc(unsigned int size);
void * kcalloc(uint32_t nmemb, uint32_t size);
void * krealloc( void *ptr, size_t size);
void kfree(void * ptr);

void * kpalloc(unsigned int npages);
void kpfree(void * address);

void alloc_print_list();

void *get_physaddr_d( void * directory, void * virtualAddr);
void map_virtaddr_d(void *directory, void * virtual_addr, void * physical_addr, uint16_t flags);
void unmap_virtaddr_d( void * directory, void * virtual_addr);

void paging_directory_list(void * dir_entry);




#endif