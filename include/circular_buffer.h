#ifndef __CIRCULAR_BUFFER_H__
#define __CIRCULAR_BUFFER_H__

#include <stdint.h>
#include <sys.h>
#include <fb.h>
#include <pmm.h>
#include <str.h>


typedef struct {
    uint8_t * base;
    size_t max_size; //could be power of 2
    size_t write;  //just index
    size_t read; //just index

} circular_buffer_t;


circular_buffer_t circular_buffer_create(size_t buffer_size);
void circular_buffer_destroy(circular_buffer_t * cb);

int circular_buffer_getc(circular_buffer_t * cb);
int circular_buffer_putc(circular_buffer_t * cb, uint8_t val);
int circular_buffer_write(circular_buffer_t * bf, void * src, uint32_t size, uint32_t nmemb);





#endif