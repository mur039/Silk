

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

int puts(char * dst){
    while(*(dst) != '\0') write( fileno_stdout, (dst++), 1);
    return 0;
}
int putchar(int c){
    return write(fileno_stdout, &c, 1);
}

const char hexadecimal_characters[17] = "0123456789abcdef";
void put_hex(unsigned int hex){
    int non_zero = 0;
    for (int i = 0; i < 8; i++)
    {
        int nibble = (hex >> (28 -(4*i))) & 0xF;
        if(non_zero == 0 && nibble != 0){
            non_zero = 1;
            }
        if(non_zero|| (i == 7 && non_zero == 0) ){
            write(fileno_stdout, &hexadecimal_characters[nibble], 1);
        }
    }
    
    return;
}



int main(int argc, char ** argv){

    int arg_count = argc;
    char ** arg_vars = &argv[1];

    

    int fd_console = open("/dev/console", O_WRONLY);
    if(fd_console == -1){
        return 1;
    }
    fileno_stdout = fd_console;

    int fd_ksock = open("/dev/ksock", O_RDWR);
    if(fd_ksock == -1){
        puts("Failed to open ksock\n");
        return 1;
    }

    
    
    write(fd_ksock, "GET_DEVICE 0", 13);
    int ch;
    while( (ch = read(fd_ksock)) != '\n' ){ //eradline basically
        putchar(ch);
    }
    while(1);

    // //fuck it we ball
    // int fd_file = open("/share/kawai.ascii", O_RDONLY);
    // int _byte = 0;
    // while( (_byte = read(fd_file)) != -1){
    //     putchar(_byte);
    // }
    // return 0;

    // if(arg_count < 2){
    //     puts("Usage : cat FILE\n");
    //     return 1;
    // }

    


    // int file_fd = open(arg_vars[1], O_RDONLY);
    // if( file_fd == -1){
    //     puts("No such file.\n");
    //     return 1;
    // }

    // stat_t st;
    // fstat(file_fd, &st);
    // int file_type = st.st_mode >> 16;

    // switch (file_type)
    // {
    //     case REGULAR_FILE:
    //     case CHARACTER_SPECIAL_FILE:
    //     case BLOCK_SPECIAL_FILE:
    //         break;
        
    //     case DIRECTORY:
    //         puts("'");puts(arg_vars[1]);puts("' is a directory.\n");
    //         return 1;
    //     default:

    //         break;
    // }

    // int byte;
    // while( (byte = read(file_fd)) != -1){
    //     putchar(byte);
    // }
    
    return 0;
}