#ifndef __PROC_H__
#define __PROC_H__

#include <filesystems/vfs.h>
#include <filesystems/tmpfs.h>
#include <process.h>

fs_node_t * proc_create();
struct fs_node* proc_finddir(struct fs_node* node, char *name);

struct proc_entry{
    char *proc_name;
    uint32_t proc_mode;
    struct fs_ops proc_ops;

    struct proc_entry* parent;
    list_t subdir; //for directories only
    void* data; 
};

int proc_init();
struct proc_entry* proc_create_entry(const char* name, uint32_t mode, struct proc_entry* parent);
void remove_proc_entry(const char* name, struct proc_entry* parent);
#endif