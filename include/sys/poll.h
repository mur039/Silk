#ifndef __K_POLL_H
#define __K_POLL_H

#include <g_list.h>
#include <filesystems/vfs.h>

#define POLLIN   0x1
#define POLLPRI  0x2
#define POLLOUT 0x4 
#define POLLRDHUP 0x8 
#define POLLERR 0x10
#define POLLHUP 0x20
#define POLLNVAL 0x40


struct fs_node;


//pollin
struct pollfd {
    int   fd;         /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
};



struct poll_table {
    void (*qproc)(struct fs_node* inode, list_t *wq, struct poll_table *pt);
    void *key; // optional flags like POLLIN vs POLLOUT
};

//helper
void poll_wait(struct fs_node* inode, list_t *wq, struct poll_table *pt);
//internal





#endif