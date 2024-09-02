#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <sys.h>
#include <pmm.h>

#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STRING "/"
#define PATH_UP  ".."
#define PATH_DOT "."

typedef struct __fs_t {
	char *name;
	uint8_t (*probe)(struct __device_t *);
	uint8_t (*read)(char *, char *, struct __device_t *, void *);
	uint8_t (*read_dir)(char *, char *, struct __device_t *, void *);
	uint8_t (*touch)(char *fn, struct __device_t *, void *);
	uint8_t (*writefile)(char *fn, char *buf, uint32_t len, struct __device_t *, void *);
	uint8_t (*exist)(char *filename, struct __device_t *, void *);
	uint8_t (*mount)(struct __device_t *, void *);
	uint8_t *priv_data;
} filesystem_t;



void vfs_init();

#endif