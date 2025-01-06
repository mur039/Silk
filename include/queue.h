#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pmm.h>
#include <vmm.h>
#include <g_list.h>
//for now it will be a linked list

struct Queue{
    list_t * list;
    size_t size;
};

typedef struct Queue queue_t;

queue_t queue_create( size_t nmemb);

int queue_enqueue_item( queue_t * queue, void * data);
void * queue_dequeue_item( queue_t * queue);


#endif