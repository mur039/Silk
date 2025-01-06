#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint-gcc.h>


int abs(int n){
    n &= ~0x80000000; //signed bit set to zero
    return n;
}

/*
well 32-bit due to size can hold values between (-2^31, 2^31) so a 31-bit
 2^31 has ~10.33 digits or 11 digits, characters first character is sign digit the rest is the integer
 for now only decimal is supported no hex or octal
*/
int atoi (const char * str){
    
    int sign = +1;
    int ret_val = 0;
    const  char *digits = str;

    if(str[0] == '-'){
        sign = -1;
        digits++;
    }



    for(size_t i = 0; i < 11 && digits[i] != '\0' ;i++){
        ret_val *= 10;
        ret_val += digits[i] - '0';
    }
    return ret_val * sign;
}

 


struct Block
{
    unsigned int is_free;
    unsigned int size;
    
    struct Block * prev;
    struct Block * next;
};

typedef struct Block block_t;

static block_t* mem_list = NULL;
void malloc_init(){
    mem_list = mmap(NULL, 0, 0, MAP_ANONYMOUS, -1, 0);
    mem_list->is_free = 1;
    mem_list->size = 4096; //raw size
    mem_list->size -= sizeof(block_t); //header size
    mem_list->next = NULL;
    mem_list->prev = NULL;

}

void* malloc(size_t size){

    for(block_t * head = mem_list; head != NULL; head = head->next){
        
        if(head->is_free && size < head->size ){


           block_t  temp;
           block_t  * next;
           
           temp = *head;

            next = head;
            next->size = size;
            next->is_free = 0;
            next->next = (block_t *)((uint8_t *)(next) + sizeof(block_t) + size);

            
            next->next->prev = next;
            next = next->next;
            next->size = temp.size - (size + sizeof(block_t));
            next->next = NULL;
            next->is_free = 1;
            


            return &next->prev[1];

        }

        if( head->next == NULL){ // jessy, we need more pages
            block_t * new = mmap(NULL, 0, 0, MAP_ANONYMOUS, -1, 0);
            new->is_free = 1;
            new->size = 4096 - sizeof(block_t);
            new->next = NULL;
            new->prev = head;
            head->next = new;
            return malloc(size);
        }
    }
    
    return NULL;
}





void* calloc(size_t nmemb, size_t size){
    void * retval;
    retval = malloc(nmemb * size);
    if(retval)
        memset(retval, 0, nmemb * size);
    
    return retval;
}

void  free(void *ptr){
    block_t * head = (block_t *)ptr;
    head -= 1;
    head->is_free = 1;

    // //merge to other free blocks
    if( head->prev->size > 0){
        head->prev->next = head->next;
        head->next->prev = head->prev;
        
        head->prev->size += head->size + sizeof(block_t);
    }
    else if(head->next->size > 0){
        
    }
    head->prev->next = head->next;
    head->next->prev = head->prev;
    return;
}



