#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <termios.h>

int main(int argc, char** argv){

    //expects a port so 
    if(argc < 2){
        printf("Expected a port darling\n");
        return 1;
    }


    int termfd = open(argv[1], O_RDWR);
    if(termfd < 0){
        printf("Failed to open port \"%s\"\n", argv[1]);
        return 1;
    }

    close(termfd); // :/

    int err = fork();
    if(err > 0){ //parent
        return 0;
    }
    else if(err < 0){
        printf("Failed to fork off\n");
        return 1;
    }


    //child
    setsid();
    int fd = open(argv[1], O_RDWR);
    
    dup2(fd, FILENO_STDIN);
    dup2(fd, FILENO_STDOUT);
    dup2(fd, FILENO_STDERR);

    char* const pargv[2] = { "-dash", NULL};
    execv("/bin/dash", pargv);

}