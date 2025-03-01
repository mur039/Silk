#include <g_list.h>



list_t list_create(){
    list_t retval;

    retval.size = 0;
    retval.head = NULL;
    retval.tail = NULL;
    return retval;
}


listnode_t * list_insert_end(list_t * list, void * val){
    
    listnode_t * t = kcalloc(1, sizeof(listnode_t));
    t->val = val;

    if(!list->head){ //empty list
        list->head = t;
        list->tail = list->head;
        list->size = 1;
        
        t->next = NULL;
        t->prev = NULL;
        return t;
    }

    //non empty list
    listnode_t* h = list->tail;
    h->next = t;
    t->prev = h;
    t->next = NULL;
    
    list->tail = t;
    list->size += 1;
    
    return t;
}


listnode_t* list_pop_end(list_t* list){
    
    //sanity check
    if(!list)
        return NULL;
    
    if(!list->size) //no elements in list
        return NULL;
    
    listnode_t* last_node = list->tail;
    
    if(list->head == list->tail){ //only one element thus

        memset(list, 0, sizeof(list_t));
    }
    else{
        
        list->tail = last_node->prev;
        list->tail->next = NULL;
        list->size -= 1;
    }

    return last_node;
}




// listnode_t * list_insert_start(list_t * list, void * val){
//     listnode_t * t = kcalloc(sizeof(listnode_t), 1);
//     t->val = val;

//     listnode_t * head = list->head;
//     if(head == NULL){
//         list->head = t;
//         t->next = NULL;
//         t->prev = list->head;    
//     }
//     else{
//         t->next = list->head;
//         list->head->prev = t;
//         list->head = t;
            
        
//     }
//     list->size++;
//     return t;
// }

listnode_t * list_remove(list_t * list, listnode_t * node){

    if(list->size == 0)
        return NULL;

     if(node->prev && node->next){ //if forward and backward exists
        node->prev->next = node->next;
        node->next->prev = node->prev;


    }else if(node->next){ //arkası boş
        list->head = node->next;
        node->next->prev = list->head;

    }else if(node->prev){ //önü boş, son eleman         
        node->prev->next = NULL;
        list->tail = node->prev;

    }

    list->size -= 1;
    if(list->size == 0){ //well
        list->head = NULL;
        list->tail = NULL;
    }

    return node; //could've been void tho, could it?
}

// void list_delete_list(list_t * list){
//     for(listnode_t * node = list->head; node; node = node->next){
//         list_remove(list, node);
//         kfree(node->val);//hmmmm
//         kfree(node);
        
//     }

// }