#include <g_list.h>

listnode_t * list_insert_front(list_t * list, void * val){

	listnode_t * t = kcalloc(sizeof(listnode_t), 1);
	// If it's the first element, then it's both head and tail
	if(!list->head){
        list->head = t;
		list->tail = t;
    }
    else{
        //check if its circular it shouldn't but check it anyway
        if(list->head->prev)
	        list->head->prev = t;
        
        t->next = list->head;
	    t->val = val;
	    list->head = t;

    }


	list->size++;
	return t;
}

#include <process.h>
listnode_t * list_insert_end(list_t * list, void * val){
    listnode_t * t = kcalloc(sizeof(listnode_t), 1);
    t->val = val;

    listnode_t * head = list->head;
    if(head == NULL){ //empty list
        list->head = t;
        t->next = NULL;
        t->prev = NULL;
        list->tail;
        // t->prev = list->head;    
    }
    else{
        for( ; head->next != NULL && head != NULL ;head = head->next); //get last node
        head->next = t;
        t->next = NULL;
        t->prev = head;
    }
    list->size += 1;




    return t;
}

listnode_t * list_insert_start(list_t * list, void * val){
    listnode_t * t = kcalloc(sizeof(listnode_t), 1);
    t->val = val;

    listnode_t * head = list->head;
    if(head == NULL){
        list->head = t;
        t->next = NULL;
        t->prev = list->head;    
    }
    else{
        t->next = list->head;
        list->head->prev = t;
        list->head = t;
            
        
    }
    list->size++;
    return t;
}

listnode_t * list_remove(list_t * list, listnode_t * node){

     if(node->prev && node->next){ //if forward and backward exists
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node = node->prev;

    }else if(node->next){ //arkası boş
        list->head = node->next;
        node->next->prev = list->head;

    }else if(node->prev){ //önü boş            
        node->prev->next = list->tail;

    }

    list->size -= 1;
    kfree(node);

    return node; //could've been void tho, could it?
}
