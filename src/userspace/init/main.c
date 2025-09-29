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

        setsid(); //become the session master darling
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
        return 1;
    }

    //while there mount proc
    mount("proc", "/proc", "proc", 0, NULL);

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
                return 3;
            }
        }
        
    }
    
    
    return 0;
}