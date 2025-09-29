#ifndef __SHAREDMEM_H
#define __SHAREDMEM_H

#include <stddef.h>

typedef unsigned long key_t;

#define IPC_CREAT (1 << 12)//Create a new segment. If this flag is not used, then shmget() will find the segment associated with key and check to see if the user has permission to access the segment.
#define IPC_EXCL (2 << 12)
#define SHM_MODE_MASK 0xfff

struct shm_segment {
    key_t key;              // SysV key
    int shmid;              // internal id
    size_t size;
    void *pages;            // allocated pages backing the segment
    int refcount;           // number of attached processes
    int flags;              // permissions etc.
};


#define SHMMIN (0x1000) //4kB
#define SHMMAX (64*0x1000) //256kB


int sharedmem_initialize(); 


#endif