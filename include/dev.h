#ifndef __DEV_H__
#define __DEV_H__

#include <stdint.h>
#include <filesystems/vfs.h>
#include <filesystems/fs.h>
#include <sys.h>
#include <pmm.h>
#include <str.h>

typedef enum __device_type {
	DEVICE_UNKNOWN = 0,
	DEVICE_CHAR = 1,
	DEVICE_BLOCK = 2,
	DEVICE_DIR = 3
} device_type;


typedef struct __device_t {
	char *name;
	uint32_t unique_id;
	device_type dev_type;
 
	struct fs_ops ops;
	void *priv;
} device_t;

//okay i need to get some directory support
//should i make it creatable by the user or
//make it like classes perhaps?

void dev_init();
int dev_register(device_t* dev);
device_t * dev_get_by_name(const char* devname);
void list_devices();


//test
fs_node_t * devfs_create();
uint32_t devfs_generic_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
uint32_t devfs_generic_write(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);

struct fs_node* devfs_finddir(struct fs_node* node, const char *name);
static struct dirent * devfs_readdir(fs_node_t *node, uint32_t index);



#endif