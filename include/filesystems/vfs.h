#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <sys.h>
#include <pmm.h>

#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STRING "/"
#define PATH_UP  ".."
#define PATH_DOT "."

void vfs_init();

#endif