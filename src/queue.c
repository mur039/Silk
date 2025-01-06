#include <queue.h>

queue_t queue_create( size_t nmemb){
    queue_t q;
    q.list = kcalloc(1, sizeof(list_t));
    q.size = nmemb;
    return q;
}

int queue_enqueue_item( queue_t * queue, void * data){
    // if(queue->list->size >= queue->size){
    //     return -1;
    // }

    //insert to the end
    list_t * l = queue->list;
    listnode_t * node = kcalloc(1, sizeof(listnode_t));
    
    node->val = data;
    node->next = NULL;

    if(!l->head){ //empty list
        l->head = node;
        l->tail = node;
    
    }
    else{
        listnode_t * last;
        last = l->tail;
        last->next = node;
        l->tail = node;
        
    }

    l->size += 1;
    return 0;

}

void * queue_dequeue_item( queue_t * queue){
    
    if( queue->list->size == 0)  //empty list
        return NULL;

    //remove from front/head
    void * value;
    list_t * l = queue->list;
    listnode_t * node = l->head;
    
    value = node->val; //retieved the value
    l->head = node->next; //modeving head to the next node

    kfree(node);
    l->size -= 1;
    return value;
}

