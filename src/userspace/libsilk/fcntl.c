#include <fcntl.h>
#include <sys/syscall.h>
#include <stdarg.h>

int open(const char * path, int flags, ...){
    
    va_list args;
    va_start(args, flags);

    if(flags & O_CREAT){
        int mode = va_arg(args, int);
        return syscall(SYSCALL_OPEN, path, flags, mode);    

    }

    return syscall(SYSCALL_OPEN, path, flags, -1);


}
