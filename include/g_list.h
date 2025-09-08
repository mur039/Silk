#ifndef __G_LIST_H__
#define __G_LIST_H__

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


list_t list_create();
listnode_t * list_insert_end(list_t * list, void * val);
listnode_t * list_remove(list_t * list, listnode_t * node);

listnode_t* list_pop_end(list_t* list);
int list_sort(list_t* list,  int (*comparefunc)(void*, void*) );
void* list_get_head_value(list_t* l, int* aux);
// listnode_t * list_insert_front(list_t * list, void * val);
// listnode_t * list_insert_start(list_t * list, void * val);
// void list_delete_list(list_t * list);

#endif