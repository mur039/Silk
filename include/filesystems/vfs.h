#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <stddef.h>
#include <g_list.h>
#include <g_tree.h>
#include <sys/poll.h>
// #include <sys.h>
// #include <pmm.h>

#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STRING "/"
#define PATH_UP  ".."
#define PATH_DOT "."



#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x04
#define FS_BLOCKDEVICE 0x08
#define FS_PIPE        0x10
#define FS_SYMLINK     0x20
#define FS_MOUNTPOINT  0x40
#define FS_SOCKET      0x80

#define	    _IFMT	0170000	/* type of file */
#define		_IFDIR	0040000	/* directory */
#define		_IFCHR	0020000	/* character special */
#define		_IFBLK	0060000	/* block special */
#define		_IFREG	0100000	/* regular */
#define		_IFLNK	0120000	/* symbolic link */
#define		_IFSOCK	0140000	/* socket */
#define		_IFIFO	0010000	/* fifo */

struct fs_node;


typedef uint32_t (*read_type_t) (struct fs_node *, uint32_t, uint32_t, uint8_t *);
typedef uint32_t (*write_type_t) (struct fs_node *, uint32_t, uint32_t, uint8_t *);
typedef void (*open_type_t) (struct fs_node *, int flags);
typedef void (*close_type_t) (struct fs_node *);
typedef struct dirent *(*readdir_type_t) (struct fs_node *, uint32_t);
typedef struct fs_node *(*finddir_type_t) (struct fs_node *, const char *name);
typedef void (*create_type_t) (struct fs_node *, const char *name, uint16_t permission);
typedef void (*mkdir_type_t) (struct fs_node *, const char *name, uint16_t permission);
typedef int (*ioctl_type_t) (struct fs_node *, unsigned long request, void * argp);
typedef int (*get_size_type_t) (struct fs_node *);
typedef int (*unlink_type_t) (struct fs_node *, const char *name);
typedef void* (*mmap_type_t) (struct fs_node *, size_t length, int prot, size_t offset);
typedef void* (*truncate_type_t) (struct fs_node *, size_t length);
typedef short (*poll_type_t) (struct fs_node *, struct poll_table*);

struct fs_ops {
	read_type_t read;
	write_type_t write;
	open_type_t open;
	close_type_t close;
	ioctl_type_t ioctl;
	mmap_type_t mmap;
	get_size_type_t get_size;
	truncate_type_t truncate;

	readdir_type_t readdir;
	finddir_type_t finddir;
	create_type_t create;
	mkdir_type_t mkdir;
	unlink_type_t unlink;

	poll_type_t poll;
};

typedef struct fs_node {
	char name[256];         // The filename.
	void * device;          // Device object (optional)
	uint32_t mask;          // The permissions mask.
	uint32_t uid;           // The owning user.
	uint32_t gid;           // The owning group.
	uint32_t flags;         // Flags (node type, etc).
	uint32_t inode;         // Inode number.
	uint32_t length;        // Size of the file, in byte.
	uint32_t impl;          // Used to keep track which fs it belongs to.
	uint16_t major_number;
	uint16_t minor_number;


	/* times */
	uint32_t atime;         // Accessed
	uint32_t mtime;         // Modified
	uint32_t ctime;         // Created

	/* File operations */
	struct fs_ops ops;

	struct fs_node *ptr;   // Alias pointer, for symlinks.
	uint32_t offset;       // Offset for read operations XXX move this to new "file descriptor" entry
	int32_t shared_with;   // File descriptor sharing XXX 
	size_t refcount;
} fs_node_t;



struct fsnode_cache_entry{
	fs_node_t * node;
	uint32_t key;
	struct fsnode_cache_entry* next;
};

struct fsnode_cache {
	size_t bucket_size;
	struct fsnode_cache_entry **buckets;
};


struct vfs_entry {
	char * name;
	list_t nodes;
	// fs_node_t * file; /* Or null */
};


typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t; 

typedef enum {
    REGULAR_FILE = 0,
    LINK_FILE,
    RESERVED_2,
    CHARACTER_SPECIAL_FILE,
    BLOCK_SPECIAL_FILE,
    DIRECTORY,
    FIFO_SPECIAL_FILE,
    RESERVED_7
} file_types_t;





struct dirent {
	uint32_t ino;           // Inode number.
    uint32_t off;
    uint16_t reclen;
    int32_t type;
	char name[256];         // The filename.
};


extern tree_t* fs_tree; /* File system mountpoint tree */
extern fs_node_t * fs_root; /* Pointer to the root mount fs_node (must be some form of filesystem, even ramdisk) */




char** _vfs_parse_path(const char * path);
void vfs_install();
int vfs_mount(char * path, fs_node_t * local_root);



void open_fs(fs_node_t *node, int flags);
void close_fs(fs_node_t *node);
void create_fs(fs_node_t* node,  const char *name, uint16_t permissions);
int mkdir_fs(fs_node_t* node, char *name, uint16_t permission);
uint32_t write_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t read_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
fs_node_t *finddir_fs(fs_node_t *node, char *name);
struct dirent *readdir_fs(fs_node_t *node, uint32_t index);
int unlink_fs(fs_node_t* node, const char* name);
int ioctl_fs(fs_node_t* node, unsigned long req, void* arg);
void vfs_tree_traverse(tree_t* tree);
fs_node_t *get_mount_point(char * path, unsigned int path_depth, char **outpath, unsigned int * outdepth);
char* vfs_canonicalize_path(const char* cwd, const char* rpath);


fs_node_t *kopen(char *filename, uint32_t flags);
list_t* vfs_directory_entry_list(fs_node_t* src_node);

void vfs_copy(fs_node_t* root_node, fs_node_t* target_node, int depth);
void vfs_traverse_node(fs_node_t* root_node, int depth);


#endif