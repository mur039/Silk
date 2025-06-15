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


    //somehow i nwwd to verify that the node is in this list??
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




//compare func will return 0 or 1, if one given pairs will be switched in place
int list_sort(list_t* list, int (*comparefunc)(void*, void*)) {
    if (!list || !list->head) return 0;

    int switches_occurred = 0;
    int did_swap;

    do {
        did_swap = 0;
        listnode_t* node = list->head;

        while (node && node->next) {
            listnode_t* a = node;
            listnode_t* b = node->next;

            if (comparefunc(a->val, b->val)) {
                // Swap a and b

                listnode_t* a_prev = a->prev;
                listnode_t* b_next = b->next;

                // Adjust head/tail
                if (list->head == a) list->head = b;
                if (list->tail == b) list->tail = a;

                // Re-link neighbors
                if (a_prev) a_prev->next = b;
                b->prev = a_prev;

                b->next = a;
                a->prev = b;

                a->next = b_next;
                if (b_next) b_next->prev = a;

                did_swap = 1;
                switches_occurred++;

                // Move node back to new 'b' (which is before a)
                node = b;
            }

            node = node->next;
        }

    } while (did_swap);

    return switches_occurred;
}
