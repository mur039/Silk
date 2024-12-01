#include <filesystems/vfs.h>


typedef struct gtreenode {
    // list_t * children;
    void * children;
    void * value;
}gtreenode_t;

typedef struct gtree {
    gtreenode_t * root;
}gtree_t;




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





typedef struct vfs_entry {
    char * name;
    vfs_node_t * file;
}vfs_entry_t;


gtree_t * vfs_tree;
void vfs_init() {
    vfs_tree = kmalloc(sizeof(gtree_t));

    struct vfs_entry * root = kmalloc(sizeof(struct vfs_entry));

    root->name = kmalloc(5);//= strdup("root");
    memcpy(root->name, "root", 5);
    root->file = NULL;  
    // tree_insert(vfs_tree, NULL, root);
}
