#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <sys.h>
#include <pmm.h>
// #include <dev.h>
#include <g_tree.h>

#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STRING "/"
#define PATH_UP  ".."
#define PATH_DOT "."



typedef struct __fs_t {
	char *name;
	uint8_t (*probe)(void  *);
	uint8_t (*read)(char *, char *, void  *, void *);
	uint8_t (*read_dir)(char *, char *, void  *, void *);
	uint8_t (*touch)(char *fn, void  *, void *);
	uint8_t (*writefile)(char *fn, char *buf, uint32_t len, void  *, void *);
	uint8_t (*exist)(char *filename, void  *, void *);
	uint8_t (*mount)(void  *, void *);
	uint8_t (*close)(void*); //only for special devices with refcounts
	uint8_t (*open)(void*); //only for special devices with refcounts
	uint8_t *priv_data;
} filesystem_t;




//node types
#define VFS_NODE_TYPE_FILE 0x01
#define VFS_NODE_TYPE_FOLDER 0x02
#define VFS_NODE_TYPE_MOUNT_POINT 0x04
#define VFS_NODE_TYPE_PIPE 0x08
#define VFS_NODE_TYPE_SYMLINK 0x10
#define VFS_NODE_TYPE_CHAR_DEVICE 0x20



struct vfs_node;

typedef struct vfs_node {
    // Baisc information about a file(note: in linux, everything is file, so the vfs_node could be used to describe a file, directory or even a device!)
    char name[256];
    void * device;
    uint32_t mask;
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t inode_num;
    uint32_t size;
    uint32_t fs_type;
    uint32_t open_flags;
    // Time
    uint32_t create_time;
    uint32_t access_time;
    uint32_t modified_time;

    uint32_t offset;
    unsigned nlink;
    int refcount;

    void  (* open)(const char * path, int mode) ;
    void (* close)(void) ;
    void  (* read)(void) ;
    void (* write)(void) ;

/*
    // File operations
    readdir_callback readdir;
    finddir_callback finddir;
    create_callback create;
    unlink_callback unlink;
    mkdir_callback mkdir;
    ioctl_callback ioctl;
    get_size_callback get_size;
    chmod_callback chmod;
    get_file_size_callback get_file_size;

    listdir_callback listdir;
    */

}vfs_node_t;




void vfs_init();
int vfs_mount(const char* path, vfs_node_t * node);
vfs_node_t* vfs_open(const char* path, uint32_t flags);
vfs_node_t* vfs_find_node_by_name(const char* path);


#endif