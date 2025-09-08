#ifndef __FS_H__
#define __FS_H__

#include <filesystems/vfs.h>


typedef struct filesystem {
    const char* fs_name;          
    int fs_flags;                 
    struct module* owner;         
    struct filesystem* next;

    //ops
    fs_node_t* (*mount)(fs_node_t* device, const char* target_path);    
    void (*unmount)(fs_node_t* fs_root);

    // optional probe function for autodetection
    int (*probe)(fs_node_t* device);

    // optional FS-specific per-mount private data
    void* fs_priv;
} filesystem_t;

int fs_register_filesystem( filesystem_t* fs);
filesystem_t* fs_get_by_name(const char* fs_name);


#endif