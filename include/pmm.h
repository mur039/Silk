#ifndef __PMM_H_
#define __PMM_H_
#include <stdint.h>
#include <sys.h>
#include <str.h>

/*only works after high-kernel initialized*/
#define BLOCK_SIZE 4096
extern uint32_t kernel_end;
void pmm_init(uint32_t memsize);
void *get_physaddr(void * virtualAddr);

union virtAddr_f{
    struct{
        uint32_t offset    :12;
        uint32_t table     :10;
        uint32_t directory :10;
    };
    uint32_t virtAddr;
};


typedef union{

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



#endif