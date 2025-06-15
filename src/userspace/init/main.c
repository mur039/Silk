#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>

//if succesfull return pid of child, else -1
int create_child_terminal(const char* filepath, const char* termpath){
    
    int pid = fork();
    
    if(pid == -1){
        return -1;
    }

    if(pid == 0){

        // setsid(); //become the session master darling
        int termfd = open(termpath, O_RDWR);
        if(termfd < 0){
            return -1;  
        }

        dup2(termfd, FILENO_STDIN);
        dup2(termfd, FILENO_STDOUT);
        dup2(termfd, FILENO_STDERR);

        fcntl(FILENO_STDIN, F_SETFL, O_RDONLY);
        fcntl(FILENO_STDOUT, F_SETFL, O_WRONLY);
        fcntl(FILENO_STDERR, F_SETFL, O_WRONLY);


        puts("As a child i execute the init program\n");
        
        const char * program_path = filepath;
        char *const args[] = { 
                                (char*)program_path,
                                "-l",
                                NULL
                                };

        int result = execve(
                        program_path,
                        args,
                        NULL
                        );

        if(result == -1){
            puts("Failed to execute another process :(\n");
            return -2;
        }
       
        
    }
    return pid;
}


int main(int _argc, char * _argv[]){    

    if(getpid() != 1){
        puts("Must be runned as PID, 1\n");
        return 1;
    }

    const char* ttypath = "/dev/tty0";
    //should inherit from the kernel but
    int fd = open(ttypath, O_RDWR);
    if(fd < 0){
        return 1;
    }

    dup2(fd, FILENO_STDIN);
    dup2(fd, FILENO_STDOUT);
    dup2(fd, FILENO_STDERR);

    int flags = fcntl(fd, F_GETFL, 0);

    fcntl(FILENO_STDIN, F_SETFL,  O_RDONLY);
    fcntl(FILENO_STDOUT, F_SETFL, O_WRONLY);
    fcntl(FILENO_STDERR, F_SETFL, O_WRONLY);

    close(fd);



    puts("Hello from init process\n");
    puts("Let's try to fork and execute init process...\n");

    int pid;
    pid = create_child_terminal("/bin/dash", "/dev/ttyS0");    
    if(pid < 0){
        puts("Failed to fork, exitting...\n");
        return 1;
    }

    pid = create_child_terminal("/bin/dash", "/dev/tty0");
    if(pid < 0){
        puts("Failed to fork, exitting...\n");
        return 1;
    }



    
    while(1){

        pid_t err = wait4(-1, NULL, 0, NULL);
        
        if( err < 0){ //some error
            
            if( errno == ECHILD){ //no child
                puts("I don't have any children left motherfucker, though i can relunch a new program but oh well\n");
                return 0;
            }
        }
        
    }
    
    
    return 0;
}