    #ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stddef.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <fcntl.h>

int read(int fd, void * buf, int count);
int close(int fd);
int exit(int code);
int execve(const char * path, const char * argv[]);
int fork();
size_t write(int fd, const void *buf, size_t count);
long lseek(int fd, long offset, int whence);
int fstat( int fd, stat_t * statbuf);
int dup2(int old_fd, int new_fd);
int getpid();
int pipe(int * fileds);
pid_t wait4(pid_t pid, int * wstatus, int options, struct rusage * rusage);
int kill(pid_t pid, int sig);
size_t getdents(int fd, void * dirp,  unsigned int count);
char* getcwd(char* buf, size_t size);
int chdir(const char* path);

int mount(const char *source, const char *target,
          const char *filesystemtype, unsigned long mountflags,
          const void * data);
        
int unlink(const char* name);
int mkdir(const char* pathname, int mode);
int ioctl(int fd, unsigned long request, void* argp);
int pause(void);

//retvals from kernel
#define EAGAIN 2
#endif