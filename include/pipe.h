#ifndef __PIPE_H__
#define __PIPE_H__
#include <circular_buffer.h>
#include <filesystems/vfs.h>
 #include <g_list.h>

typedef struct _pipe_device {
	uint8_t * buffer;
	size_t write_ptr;
	size_t read_ptr;
	size_t size;
	size_t refcount;
	uint8_t volatile lock;
	list_t * wait_queue;
} pipe_device_t;

fs_node_t * create_pipe(size_t size);
int pipe_size(fs_node_t * node);

#endif