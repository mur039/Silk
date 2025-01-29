#include <filesystems/ext2.h>

fs_node_t * ext2_node_create(device_t* blk_device){

    if(blk_device->dev_type != DEVICE_BLOCK) return NULL;

    //should probe the drive for ext2 specific information

    fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));

    strcpy(fnode->name, "ext2");
    fnode->inode = 0;
    fnode->impl = 6;
    fnode->flags = FS_DIRECTORY;

    return fnode;
}
