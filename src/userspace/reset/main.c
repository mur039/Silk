#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <termios.h>


int main(int argc, char** argv){
    
    //hmm well we need set set stdin stdout and stderr to a sane options

    for(int i = 0; i < 3; ++i){
        
        if(!isatty(i)) continue; //weird but ok
        struct termios term;
        tcgetattr(i, &term);

        term.c_cflag = ISIG | ICANON | ECHO;
        tcsetattr(i, TCSANOW, &term);
    }

    puts("\e[H\e[0m");
    return 0;    
}