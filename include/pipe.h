#ifndef __PIPE_H__
#define __PIPE_H__
#include <circular_buffer.h>
#include <filesystems/tar.h>

typedef struct{
    circular_buffer_t cb;
    size_t read_refcount;
    size_t write_refcount;
} pipe_t;

file_t pipe_create(size_t size);
pipe_t * pipe_get_pntr_from_tar( tar_header_t * header);



#endif