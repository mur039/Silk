#ifndef __ARENA_H__
#define __ARENA_H__

#include <stdlib.h>
#include <stdint-gcc.h>
#include <string.h>

/*
as for an arena allocator it allows memory allocation within and no release within
whole memory is released

*/

typedef struct{
    uint8_t* internal_block;
    size_t internal_block_size;
    size_t head;
} arena_t;

int arena_create( arena_t* arena, size_t size);
int arena_destroy( arena_t* arena);
void* arena_malloc( arena_t* arena, size_t size );
void* arena_calloc( arena_t* arena, size_t nmemb, size_t size );



#endif