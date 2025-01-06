#ifndef __MMAN_H__
#define __MMAN_H__

#include <sys/syscall.h>
#include <stddef.h>

//flags
#define MAP_ANONYMOUS 1

void *mmap(void *addr , size_t length, int prot, int flags, int fd, size_t offset);



#endif