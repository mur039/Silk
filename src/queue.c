#include <queue.h>

queue_t queue_create( size_t nmemb){
    queue_t q;
    q.list = kcalloc(1, sizeof(list_t));
    q.size = nmemb;
    return q;
}

int queue_enqueue_item( queue_t * queue, void * data){

    //insert to the end
    list_t * l = queue->list;
    list_insert_end(l, data);
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

