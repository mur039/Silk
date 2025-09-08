#ifndef __PROC_H__
#define __PROC_H__

#include <filesystems/vfs.h>
#include <filesystems/tmpfs.h>
#include <process.h>

fs_node_t * proc_create();
struct fs_node* proc_finddir(struct fs_node* node, char *name);


#endif