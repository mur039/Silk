

#ifndef NULL
#define NULL (void*)0
#endif

#include <stat.h>


typedef unsigned int size_t;
extern int read(int fd);
extern int close(int fd);
extern int exit(int code);
extern int execve(const char * path);
extern size_t write(int fd, const void *buf, size_t count);
extern int fstat(int fd, stat_t * st);


typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;

extern int open(const char * s, int mode);

typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;

extern long lseek(int fd, long offset, int whence);

typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
} pixel_t;


int fileno_stdout = 0;
int fileno_stderr = 0;

int log_err(char *dst){
    while(*(dst) != '\0') write( fileno_stderr, (dst++), 1);
    return 0;
}
int puts(char * dst){
    while(*(dst) != '\0') write( fileno_stdout, (dst++), 1);
    return 0;
}
int putchar(int c){
    return write(fileno_stdout, &c, 1);
}


int main(int argc, char ** argv){

    int arg_count = argc;
    char ** arg_vars = &argv[1];

    int fd_console = open("/dev/console", O_WRONLY);
    if(fd_console == -1){
        return 1;
    }

    fileno_stdout = fd_console;

    if(arg_count < 2){
        puts("Usage : cat FILE\n");
        return 1;
    }

    


    int file_fd = open(arg_vars[1], O_RDONLY);
    if( file_fd == -1){
        puts("No such file.\n");
        return 1;
    }

    stat_t st;
    fstat(file_fd, &st);
    int file_type = st.st_mode >> 16;

    switch (file_type)
    {
        case REGULAR_FILE:
        case CHARACTER_SPECIAL_FILE:
        case BLOCK_SPECIAL_FILE:
            break;
        
        case DIRECTORY:
            puts("'");puts(arg_vars[1]);puts("' is a directory.\n");
            return 1;
        default:

            break;
    }

    int byte;
    while( (byte = read(file_fd)) != -1){
        putchar(byte);
    }
    
    return 0;
}