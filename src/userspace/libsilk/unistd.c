#include <unistd.h>
#include <sys/syscall.h>



size_t write(int fd, const void *buf, size_t count){
    return syscall(SYSCALL_WRITE, fd, buf, count);   
}

int open(const char * s, int mode){
    return syscall(SYSCALL_OPEN, s, mode );
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



