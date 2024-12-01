#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stddef.h>
#include <sys/stat.h>
#include <sys/wait.h>

int read(int fd, void * buf, int count);
int close(int fd);
int exit(int code);
int execve(const char * path, const char * argv[]);
int fork();
int open(const char * s, int mode);
size_t write(int fd, const void *buf, size_t count);
long lseek(int fd, long offset, int whence);
int fstat( int fd, stat_t * statbuf);
int dup2(int old_fd, int new_fd);
int getpid();
int pipe(int * fileds);
pid_t wait4(pid_t pid, int * wstatus, int options, struct rusage * rusage);


#endif