#ifndef __DEV_H__
#define __DEV_H__

#include <stdint.h>
#include <sys.h>
#include <fb.h>
#include <pmm.h>
#include <str.h>
#include <filesystems/vfs.h>

typedef enum __device_type {
	DEVICE_UNKNOWN = 0,
	DEVICE_CHAR = 1,
	DEVICE_BLOCK = 2,
} device_type;


typedef struct __device_t {
	char *name;
	uint32_t unique_id;
	device_type dev_type;
	filesystem_t * fs;
	// struct __fs_t *fs;
	int32_t (*read)(uint8_t* buffer, uint32_t offset , uint32_t len, void* dev);
	int32_t (*write)(uint8_t *buffer, uint32_t offset, uint32_t len, void* dev);
	uint8_t (*close)(int t, void* dev);
	uint8_t (*open) (int t, void* dev);
	void *priv;
} device_t;

void dev_init();
int dev_register(device_t* dev);
void list_devices();

#endif