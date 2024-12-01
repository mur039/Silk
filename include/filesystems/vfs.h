#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <sys.h>
#include <pmm.h>
// #include <dev.h>

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
	uint8_t *priv_data;
} filesystem_t;



void vfs_init();

#endif