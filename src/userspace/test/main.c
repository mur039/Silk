#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>


int main(){
 
    int err;
    
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t socklen = sizeof(server_addr);

        // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(128);
    server_addr.sin_addr = 0;

    err = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err < 0){
        return 1;
    }


    while(1){
        struct pollfd fds[2];

        fds[0].events = POLLIN;
        fds[1].events = POLLIN;

        fds[0].fd = STDIN_FILENO;
        fds[1].fd = sockfd;
    
        
        int err = poll(fds, 2, 1000 ); //block
        
        for(int i = 0; i < 2; ++i){
            printf("fd %u : revents %x\n", fds[i].fd, fds[i].revents);
        }

        

        if(fds[0].revents & POLLIN){ //command line
            
            fds[0].revents = 0;
            char buffer[128];
            
            int count = read(STDIN_FILENO, buffer, 128); //canonical mode
            buffer[count] = '\0';

            if(server_addr.sin_addr != 0){
                sendto(sockfd, buffer, count, 0, (struct sockaddr*)&server_addr, socklen);
            }
        }
        
        if(fds[1].revents & POLLIN){ //socket
            
            fds[0].revents = 0;
            unsigned char buffer[64];
            err = recvfrom(sockfd, &buffer, 64, 0, (struct sockaddr*)&server_addr, &socklen);
            if(err < 0){
                return 1;
            }

            buffer[err] = '\0';
            printf("%s", buffer);
        }

    }

    return 0;
}