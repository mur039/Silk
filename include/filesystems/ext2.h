#ifndef __EXT2_H__
#define __EXT2_H__

#include <dev.h>
#include <filesystems/vfs.h>
#include <fb.h>

fs_node_t * ext2_node_create(device_t* blk_device);






#endif