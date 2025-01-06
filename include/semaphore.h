#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <sys.h>
#include <pmm.h>
#include <vmm.h>
#include <queue.h>

struct Semaphore {
    int value;
    queue_t queue;
};

typedef struct Semaphore semaphore_t;

semaphore_t semaphore_create(int initial_value);



#endif