#include <pipe.h>

#include <process.h>
uint32_t read_pipe(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t write_pipe(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void open_pipe(fs_node_t *node, uint8_t read, uint8_t write);
void close_pipe(fs_node_t *node);







static inline size_t pipe_unread(pipe_device_t * pipe) {
	if (pipe->read_ptr == pipe->write_ptr) {
		return 0;
	}
	if (pipe->read_ptr > pipe->write_ptr) {
		return (pipe->size - pipe->read_ptr) + pipe->write_ptr;
	} else {
		return (pipe->write_ptr - pipe->read_ptr);
	}
}


int pipe_size(fs_node_t * node) {
	pipe_device_t * pipe = (pipe_device_t *)node->device;
	return pipe_unread(pipe);
}






static inline void pipe_increment_read(pipe_device_t * pipe) {
	pipe->read_ptr++;
	if (pipe->read_ptr == pipe->size) {
		pipe->read_ptr = 0;
	}
}

static inline void pipe_increment_write(pipe_device_t * pipe) {
	pipe->write_ptr++;
	if (pipe->write_ptr == pipe->size) {
		pipe->write_ptr = 0;
	}
}

size_t pipe_available(pipe_device_t* pipe){

	//a sane version
	if(pipe->write_ptr > pipe->read_ptr){
		return (pipe->write_ptr - pipe->read_ptr);
	}
	else if(pipe->write_ptr < pipe->read_ptr){
		return ( pipe->size - (pipe->read_ptr - pipe->write_ptr) );
	}
	else{ //write == read
		return pipe->size;
	}

}


uint32_t read_pipe(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {

	pipe_device_t * pipe = (pipe_device_t *)node->device;

	
    size_t collected = 0;
	size_t remaining = pipe_unread(pipe);
    
	if(!pipe->writerefcount && !remaining){ //no writers as well as no data then
		return 0;
	}
	
	if( size <=  remaining){

        while(pipe_unread(pipe) > 0  && collected < size){

            buffer[collected] = pipe->buffer[pipe->read_ptr];
            pipe_increment_read(pipe);
            collected++;
        }
        return collected;
    }
    
	
    list_insert_end(pipe->wait_queue, current_process);
    return -1;

}

#include  <process.h>
uint32_t write_pipe(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
	

	/* Retreive the pipe object associated with this file node */
	pipe_device_t * pipe = (pipe_device_t *)node->device;

	//if there's no reader what's the point of writing?
	if(!pipe->readrefcount){
		//as well as send SIGPIPE!!
		return -6;
	}

	size_t avaliable = pipe_available(pipe);

	if( size <= avaliable){
		for(size_t i = 0; i < size; ++i){
			pipe->buffer[pipe->write_ptr] = buffer[i];
			pipe_increment_write(pipe);
		}
		//wake up the waiters
		process_wakeup_list(pipe->wait_queue);
		return size;
	}

	//if there's no size left then 
	return -1;
}



void open_pipe(fs_node_t * node, uint8_t read, uint8_t write) {
	
	// fb_console_printf("open_pipe: %x %u %u\n", node, read, write);
    if(!node->device){
        fb_console_printf("Tried to open a fully closed pipe\n");
        return;
    }
    
	pipe_device_t * pipe = (pipe_device_t *)node->device;

	//well assumption that only one of them is 1 at a time so

	if(read){
		node->write = NULL;
		node->read = read_pipe;
		pipe->readrefcount++;
		return;
	}
	
	if(write){
		node->write = write_pipe;
		node->read = NULL;
		pipe->writerefcount++;
	}

	return;
}


void close_pipe(fs_node_t * node) {
	pipe_device_t * pipe = (pipe_device_t *)node->device;
	
	// fb_console_printf("close_pipe: %x ", node);	

	if(node->read){
		// fb_console_printf(" read\n");	
		pipe->readrefcount--;
	}

	if(node->write){
		// fb_console_printf(" write\n");	
		pipe->writerefcount--;
	}

	/* Check the reference count number */
	if (pipe->readrefcount == 0 && pipe->writerefcount == 0) { //deallocate pipe

		kfree(pipe->wait_queue);
		kfree(pipe->buffer);
		kfree(pipe);

	}

	return;
}

fs_node_t* create_pipe(size_t size){

    fs_node_t* fnode = kcalloc(1, sizeof(fs_node_t));
    pipe_device_t* pipe = kcalloc(1, sizeof(pipe_device_t));
    
	fnode->device = NULL;

	fnode->uid   = 0;
	fnode->gid   = 0;
	fnode->flags = FS_PIPE;

	fnode->read  = NULL;
	fnode->write = NULL;
	fnode->open  = open_pipe;
	fnode->close = close_pipe;
	fnode->readdir = NULL;
	fnode->finddir = NULL;
	fnode->ioctl   = NULL; /* TODO ioctls for pipes? maybe */
	fnode->get_size = pipe_size;

	fnode->atime = (uint32_t)get_ticks();
	fnode->mtime = fnode->atime;
	fnode->ctime = fnode->atime;

	fnode->device = pipe;

	pipe->buffer    = kmalloc(size);
	pipe->write_ptr = 0;
	pipe->read_ptr  = 0;
	pipe->size      = size;
	pipe->writerefcount  = 0;
	pipe->readrefcount  = 0;
	pipe->lock      = 0;

	pipe->wait_queue = kmalloc(sizeof(list_t));
    *pipe->wait_queue = list_create();
    
    return fnode;
}