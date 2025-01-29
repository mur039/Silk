#ifndef __NULLDEV_H__
#define __NULLDEV_H__

#include <filesystems/vfs.h>


uint32_t read_null(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t write_null(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void open_null(fs_node_t *node, uint8_t read, uint8_t write);
void close_null(fs_node_t *node);

uint32_t read_null(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t write_null(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void open_null(fs_node_t * node, uint8_t read, uint8_t write);
void close_null(fs_node_t * node);
fs_node_t * null_device_create();




#endif