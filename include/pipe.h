#ifndef __PIPE_H__
#define __PIPE_H__
#include <circular_buffer.h>
#include <filesystems/tar.h>

typedef struct{
    circular_buffer_t cb;
    int read_refcount;
    int write_refcount;
} pipe_t;

file_t pipe_create(size_t size);
pipe_t * pipe_get_pntr_from_tar( tar_header_t * header);

int32_t pipe_read(uint8_t * buffer, uint32_t offset, uint32_t len, void *d);
int32_t pipe_write(uint8_t * buffer, uint32_t offset, uint32_t len, void *d);
uint8_t pipe_close(int t, void *d);
uint8_t pipe_open(int t, void *d);


#endif