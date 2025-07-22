#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <termios.h>


int main(int argc, char** argv){
    
    //i only list the working directory
    int fdptmx = open("/dev/pts/ptmx", O_RDWR);

    if(fdptmx < 0){
        printf("failed to open /dev/ptmx\n");
        return 1;
    }
    //grantpt unlockpt ptsname etc
    printf("ptsname of the slave that is: %s\n", ptsname(fdptmx));

    char lbuff[128]= {0};
    int  index = 0;
    while(( index = read(fdptmx, lbuff, 128)) > 0){
        lbuff[index] = '\0';
        puts(lbuff);

    }
    

    close(fdptmx);
    return 0;    
}
