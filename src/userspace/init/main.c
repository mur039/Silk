

#ifndef NULL
#define NULL (void*)0
#endif

typedef unsigned int size_t;
extern int read(int fd);
extern int close(int fd);
extern int exit(int code);
extern int execve(const char * path, const char * argv[]);

extern size_t write(int fd, const void *buf, size_t count);
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

int getline(char * dst){
    int c = 0, i = 0;
    while(1){
        c = read(fileno_stdout);
        if(c == -1) continue;
        if(c == '\n'){
            dst[i] = '\0';
             return i;
             }

        dst[i] = c;
        i += 1;
    }
}

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

#define COMMAND_BUFFER_MAXSIZE 64
int command_buffer_index = 0;
char command_buffer[COMMAND_BUFFER_MAXSIZE];

int main(char ** argv){



    int fd_com1 = open("/dev/com1", O_RDWR);
    if(fd_com1 == -1){
        return 1;
    }

    int fd_console = open("/dev/console", O_WRONLY);
    if(fd_console == -1){
        return 1;
    }
    fileno_stdout = fd_console;
    fileno_stderr = fd_com1;



    puts("Hello from init process\n");
    puts("Let's try to execute another process...\n");

    const char * program_path = "/bin/sh";
    const char * args[] = { 
                            program_path,
                            NULL
                            };
                
    int result = execve(
                        program_path,
                        args
                        );
                        
    if(result == -1){
        puts("Failed to execute another process :(\n");
        // exit(1);
    }
    

    int fd_fb = open("/dev/fb", O_WRONLY);
    if(fd_fb == -1){
        return 1;
    }

    pixel_t pixel = {.alpha = 0xff, .blue = 0, .green = 0, .red = 0};
    while(1){
        write(fd_fb, &pixel, 1);
        pixel.red -= 1;
        pixel.green -= 1;
        pixel.blue -= 1;

    }
    
    return 0;
}