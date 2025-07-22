#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <termios.h>



struct termios original;

void restore_input_attributes(){
    tcsetattr(FILENO_STDIN, 0, &original);
}


void save_input_attributes(){
    tcgetattr(FILENO_STDIN, &original);
    atexit(restore_input_attributes);
}

void set_input(){
    struct termios tmp;
    tcgetattr(FILENO_STDIN, &tmp);
    tmp.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCOON, &tmp);
    
    int flags = fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flags);
}

int forkpty(int* fdmaster, char* path){

    if(!fdmaster){
        return -1; //??
    }

    int master = open("/dev/pts/ptmx", O_RDWR);
    if(master < 0){
        puts("failed to open master device\n");
        errno = ENOENT;
        return 1;
    }

    int pid = fork();

    if(pid == 0){ //child
        
        setsid(); //become the session master thus lose controlling terminal
        char *slave_path = ptsname(master);
        printf("mux: slave_path: %s\n", slave_path);

        int tty = open(slave_path, O_RDWR);
        if(tty < 0){
            puts("failed to open the slave terminal\n");
            return 1;
        }
        dup2(tty, FILENO_STDIN);
        dup2(tty, FILENO_STDOUT);
        dup2(tty, FILENO_STDERR);

        char* const argv[2] = { NULL};
        printf("execve, path: %s\n", path);
        execv(path, argv);
        puts("failed to execv\n");
        return 1;
    }
    else if(pid > 0){
        *fdmaster = master;
    }
    else{ //failure
        puts("failed to fork\n");
        return 1;
    }
    
    return 0;
}

int main(int argc, char** argv){

    
    save_input_attributes();
    set_input();
    //i only list the working directory 
    
    int master;
    if( forkpty(&master, "/bin/sh") ){
        puts("Failed to forkpty\n");
        return 1;
    }
    fcntl( master, F_SETFL, fcntl(master, F_GETFL) | O_NONBLOCK);

    while(1){
        char c;
        int r = read(STDIN_FILENO, &c, 1);
        
        if(!r){ //eof
            
            write(master, "\x4", 1);
            return 0;
        }
        else if(r > 0){ // i have an input
            write(master, &c, 1);
        }
        else{ //no input, i hope

        }

        //read from slave and print it to the screen
        
        char lbuff[128];
        r = read(master, lbuff, 128);
        if(r > 0){

            write(STDOUT_FILENO, lbuff, r);
        }
        

        //check if the child died?
        r = wait4(-1, NULL, WNOHANG, NULL);
        if(r < 0 && errno == ECHILD){
            return 0;
        }
    }


    return 0;    
}
