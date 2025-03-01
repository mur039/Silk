#ifndef __DEV_H__
#define __DEV_H__

#include <stdint.h>
#include <filesystems/vfs.h>
#include <sys.h>
#include <pmm.h>
#include <str.h>

typedef enum __device_type {
	DEVICE_UNKNOWN = 0,
	DEVICE_CHAR = 1,
	DEVICE_BLOCK = 2,
} device_type;


typedef struct __device_t {
	char *name;
	uint32_t unique_id;
	device_type dev_type;
	// filesystem_t * fs;
	read_type_t read;
	write_type_t write;
	open_type_t open;
	close_type_t close;
	readdir_type_t readdir;
	finddir_type_t finddir;
	create_type_t create;
	mkdir_type_t mkdir;
	ioctl_type_t ioctl;
	get_size_type_t get_size;
	void *priv;
} device_t;

void dev_init();
int dev_register(device_t* dev);
device_t * dev_get_by_name(const char* devname);
void list_devices();


//test
fs_node_t * devfs_create();
read_type_t devfs_generic_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
write_type_t devfs_generic_write(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);

finddir_type_t devfs_finddir(struct fs_node* node, char *name);
static struct dirent * devfs_readdir(fs_node_t *node, uint32_t index);



#endif