#include <unistd.h>
#include <sys/syscall.h>



size_t write(int fd, const void *buf, size_t count){
    return syscall(SYSCALL_WRITE, fd, buf, count);   
}

int read(int fd, void * buf, int count){
    return syscall(SYSCALL_READ, fd, buf, count);
}

int close(int fd){
    return syscall(SYSCALL_CLOSE, fd);
}

int exit(int code){
    return syscall(SYSCALL_EXIT, code);
}

int execve(const char * path, const char * argv[]){
    return syscall(SYSCALL_EXECVE, path, argv);
}

int fork(){
    return syscall(SYSCALL_FORK);
}

long lseek(int fd, long offset, int whence){
    return syscall(SYSCALL_LSEEK, fd, offset, whence);
}

int fstat( int fd, stat_t * statbuf){
    return syscall(SYSCALL_FSTAT, fd, statbuf);
}

int dup2(int old_fd, int new_fd){
    return syscall(SYSCALL_DUP2, old_fd, new_fd);
}

int getpid(){
    return syscall(SYSCALL_GETPID);
}

int pipe(int * fileds){
    return syscall(SYSCALL_PIPE, fileds);
}

pid_t wait4(pid_t pid, int * wstatus, int options, struct rusage * rusage){
    return syscall(SYSCALL_WAIT4, pid, wstatus, options, rusage) ;
}

void *mmap(void *addr , size_t length, int prot, int flags, int fd, size_t offset){
    return syscall(SYSCALL_MMAP, addr, length, prot, flags, fd, offset);
    }

int kill(pid_t pid, int sig){
    return syscall(SYSCALL_KILL, pid, sig);
}

char* getcwd(char* buf, size_t size){
    long result = syscall(SYSCALL_GETCWD, buf, size);
    //err
    if(result & 0x80000000){
        return NULL;
    }

    return result;

}

int chdir(const char* path){

    //do jack-shit
    long result = syscall(SYSCALL_CHDIR, path);
    return result;

}

size_t getdents(int fd, void * dirp,  unsigned int count){
    return syscall(SYSCALL_GETDENTS, fd, dirp, count);

}

int sysinfo(struct sysinfo *info){
    return syscall(SYSCALL_SYSINFO, info);
}


/*
*/
int mount(const char *source, const char *target,
          const char *filesystemtype, unsigned long mountflags,
          const void * data)
{

    return syscall(SYSCALL_MOUNT, source ,target, filesystemtype, mountflags, data);
}


int unlink(const char* pathname){
    return syscall(SYSCALL_UNLINK, pathname);
}

int mkdir(const char* pathname, int mode){
    return syscall(SYSCALL_MKDIR, pathname, mode);
}











