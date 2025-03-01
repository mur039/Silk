#ifndef __TMPFS_H__
#define __TMPFS_H__

#include <filesystems/vfs.h>
#include <fb.h>



fs_node_t * tmpfs_install();
open_type_t tmpfs_open(fs_node_t* node, uint8_t read, uint8_t write);
close_type_t tmpfs_close(fs_node_t* node);
finddir_type_t tmpfs_finddir (struct fs_node* node, char *name);
create_type_t tmpfs_create(fs_node_t* node, char* name, uint16_t permissions);
mkdir_type_t tmpfs_mkdir(fs_node_t* node, char* name, uint16_t permissions);
unlink_type_t tmpfs_unlink(fs_node_t* node, char* name);
struct dirent * tmpfs_readdir(fs_node_t *node, uint32_t index);
write_type_t tmpfs_write(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
read_type_t tmpfs_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
#endif