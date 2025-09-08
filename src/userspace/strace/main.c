#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/ptrace.h>


#define PTRACE_TRACEME 0 

#define PTRACE_ATTACH 64

int main(int argc, const char* argv[]){
 
    if(argc != 2){
        printf("Expected path to a executable\n");
        return 1;
    }

    pid_t pid;
    pid = fork();

    if(pid < 0){
        printf("Failed to fork off\n");
        return 1;
    }

    if(pid == 0){ //that's me the child
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        //stops itself
        
        return 0;
    }


    //i should trace that motherfucker
    ptrace(PTRACE_ATTACH, pid, NULL, NULL);


    return 0;
}