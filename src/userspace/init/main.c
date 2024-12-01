#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
extern int dup2(int old_fd, int new_fd);
extern int getpid();

extern size_t write(int fd, const void *buf, size_t count);
typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;

int fileno_stdout = 0;
int fileno_stderr = 0;

int log_err(char *dst){
    while(*(dst) != '\0') write( fileno_stderr, (dst++), 1);
    return 0;
}
int puts(char * dst){

    write( fileno_stdout, dst, strlen(dst));
    return 0;
}
int putchar(int c){
    return write(fileno_stdout, &c, 1);
}

char line_buf[32];

int main(int _argc, char * _argv[]){    

    //should inherit from the kernel but
    int fd_console = open("/dev/console", O_WRONLY);
    if(fd_console == -1){
        return 1;
    }


    int fd_kbd = open("/dev/kbd", O_RDONLY);
    if(fd_kbd == -1){
        return 1;
    }

    int fd_com1 = open("/dev/com1", O_RDWR);
    if(fd_com1 == -1){
        return 1;
    }


    fileno_stdout = fd_console;
    fileno_stderr = fd_com1;



    if(getpid() != 0){
        puts("Must be runned as PID, 0\n");
        return 1;
    }


    puts("Hello from init process\n");
    puts("Let's try to fork and execute init process...\n");
    
    int pid = fork();
    
    if(pid == -1){
        puts("Failed to fork, exitting...\n");
        exit(1);
    }

    if(pid == 0){
        puts("As a child i execute the init program\n");
        
        
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
            return 1;
        }
       
        
    }



    // wait4(-1, NULL, 0, NULL);
    puts("i shouldn't see this before the child\n");
    while(1);
    
    
    
    //if we return that means child has exited thus means no other process left so it would terminate
    printf("Child exited\n");

    return 0;
}