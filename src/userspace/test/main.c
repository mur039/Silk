#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <poll.h>
#include <sys/socket.h>

#define KB 1024
#define FILE_SIZE 16 * KB

int main(){

    int sfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    struct sockaddr_un s;
    s.sun_family = AF_UNIX;
    sprintf(s.sun_path, "dispsrv");

    connect(sfd, (struct sockaddr*)&s, sizeof(s));


    int err;
    char lbuff[48];
    while (1){
        err = read(sfd, lbuff, 48); //DGRAM!
        if(err < 0){
            close(sfd);
            exit(1);
        }
        printf("[client]: read \"%s\"\n", lbuff);
        break;
    }
    

    // int file = open("buff", O_RDWR | O_CREAT);
    // if(file < 0){
    //     perror("open");
    //     return 1;
    // }

    // ftruncate(file, FILE_SIZE);
    
    // void* addr = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
    // if(addr == (void*)-1){
    //     perror("mmap");
    //     return 1;
    // }


    // //if we mapped it then lets fill it with 0x69
    // uint8_t* d = addr;
    // for(int i = 0; i < FILE_SIZE; ++i){
    //     d[i] = 0x69;
    // }


    return 0;
}