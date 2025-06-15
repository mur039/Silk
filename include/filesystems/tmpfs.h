#ifndef __TMPFS_H__
#define __TMPFS_H__

#include <filesystems/vfs.h>
#include <fb.h>



fs_node_t * tmpfs_install();
void tmpfs_open(fs_node_t* node, uint8_t read, uint8_t write);
void tmpfs_close(fs_node_t* node);
struct fs_node* tmpfs_finddir (struct fs_node* node, char *name);
void tmpfs_create(fs_node_t* node, char* name, uint16_t permissions);
void tmpfs_mkdir(fs_node_t* node, char* name, uint16_t permissions);
int tmpfs_unlink(fs_node_t* node, char* name);
struct dirent * tmpfs_readdir(fs_node_t *node, uint32_t index);
uint32_t tmpfs_write(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
uint32_t tmpfs_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
#endif