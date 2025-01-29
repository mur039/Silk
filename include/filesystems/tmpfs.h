#ifndef __TMPFS_H__
#define __TMPFS_H__

#include <filesystems/vfs.h>
#include <fb.h>
struct tmpfs_internal_struct{
    tree_t* internal_tree;
    tree_node_t* internal_tree_node;
    void* priv;
    size_t buffer_size;
};



fs_node_t * tmpfs_install();
finddir_type_t tmpfs_finddir (struct fs_node* node, char *name);
create_type_t tmpfs_create(fs_node_t* node, char* name, uint16_t permissions);
mkdir_type_t tmpfs_mkdir(fs_node_t* node, char* name, uint16_t permissions);
struct dirent * tmpfs_readdir(fs_node_t *node, uint32_t index);
write_type_t tmpfs_write(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
read_type_t tmpfs_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
#endif