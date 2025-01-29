#ifndef __SYSCALL_H__
#define __SYSCALL_H__


enum syscall_numbers{
    SYSCALL_READ = 0,
    SYSCALL_WRITE = 1,
    SYSCALL_OPEN = 2,
    SYSCALL_CLOSE = 3,
    SYSCALL_LSEEK = 4,
    SYSCALL_FSTAT = 5,
    SYSCALL_MMAP = 9,
    SYSCALL_GETPID = 20,
    SYSCALL_PIPE = 22,
    SYSCALL_DUP2 = 41,
    SYSCALL_FORK = 57,
    SYSCALL_EXECVE = 59,
    SYSCALL_EXIT = 60,
    SYSCALL_WAIT4 = 61,
    SYSCALL_KILL = 62,
    SYSCALL_GETDENTS = 78,
    SYSCALL_GETCWD = 79,
    SYSCALL_CHDIR = 80,
    SYSCALL_MKDIR = 83,
    SYSCALL_UNLINK=86,
    SYSCALL_SYSINFO = 99,
    SYSCALL_MOUNT = 165,
};






// long syscall(long syscall_number, ...);

// Wrapper macro to pad missing arguments with 0
#define syscall(...) call_syscall_internal( __VA_ARGS__, 0, 0, 0, 0, 0, 0)

// Helper macro to extract the first 6 arguments
#define call_syscall_internal(syscall_number, a, b, c, d, e, f, ...) _syscall(syscall_number, (unsigned int )a, (unsigned int )b, (unsigned int )c, (unsigned int )d, (unsigned int )e, (unsigned int )f)

extern long _syscall(long syscall_number, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5);

#endif