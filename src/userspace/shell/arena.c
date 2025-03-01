#include <arena.h>



int arena_create( arena_t* arena, size_t size){
    arena->head = 0;
    arena->internal_block_size = size;
    arena->internal_block = malloc(size);
    return 0;
}

int arena_destroy( arena_t* arena){
    arena->head = 0;
    arena->internal_block_size = 0;
    free(arena->internal_block);
    return 0;
}

void* arena_malloc( arena_t* arena, size_t size ){

    if( size > (arena->internal_block_size - arena->head) ) return NULL; //no space left
    void* retval = &arena->internal_block[arena->head];
    arena->head += size;
    return retval;
}


//same shit but memory is zeroed
void* arena_calloc( arena_t* arena, size_t nmemb, size_t size ){
    void* mem = arena_malloc(arena, nmemb * size);
    
    if(mem)
        memset(mem, 0, nmemb*size);
    
    return mem;
}
