#ifndef __G_LIST_H__
#define __G_LIST_H__

#include <sys.h>
#include <pmm.h>
typedef struct listnode {
	struct listnode * prev;
	struct listnode * next;
	void * val;

}listnode_t;


typedef struct list{
	listnode_t * head;
	listnode_t * tail;
	unsigned int size;
} list_t;


listnode_t * list_insert_front(list_t * list, void * val);
listnode_t * list_insert_end(list_t * list, void * val);
listnode_t * list_insert_start(list_t * list, void * val);

#endif