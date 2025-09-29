#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdint.h>
#include <getopt.h>

int main(int argc, char* argv[]){

    if(argc != 3){
        errno = EINVAL  ;
        perror("to few arguments:");
        exit(1);
    }

    //well assumptions my dear
    const char* ipstring = argv[1];
    const char* portstring = argv[2];


    //ipstring: %u.%u.%u.%u
    unsigned short ipvals[4] = {0, 0, 0, 0};
    unsigned short port;
    const char* base = ipstring;
    const char* head = base;
    for(int i = 0; i < 4; ++i){
        
        while(*head){
            //there can be max 3, min 1 digit number
            if(isdigit(*head) ){
                if((head - base) > 2) {
                    errno = EINVAL;;
                    perror("subaddress");
                    
                    exit(1);
                }
                
                ipvals[i] *= 10;
                ipvals[i] += *head - '0';
            }
            else if(*head == '.'){
                //okay head must be greater than 1
                if((head - base) < 1){
                    errno = EINVAL;
                    perror("Expected number before delimeter");
                }
                base = head + 1;
                head++;
                break;
            }
            else{
                errno = EINVAL;;
                perror("ip address");
                exit(1);
            }
            head++;
        }
        if(i < 3 && *head == '\0'){
            //ended early then
            errno = EINVAL;
            perror("Expected 4 octets");
            exit(1);
        }

    }

    for(int i = 0; i < 4; ++i){
        if(ipvals[i] > 255){
            errno = EINVAL;
            perror("octet cannot be greater than 255");
            exit(1);
        }
    }


    port = atoi(portstring);
    unsigned long ipaddr =  (ipvals[3] << 24) | (ipvals[2] << 16) | (ipvals[1] << 8) |(ipvals[0]);
    // printf("ip address %u.%u.%u.%u:%u\n", ipvals[0], ipvals[1], ipvals[2], ipvals[3], port);
    

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
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = 0;

    err = bind(sockfd, (const struct sockaddr*)&server_addr, socklen);
    if(err < 0){
        perror("bind");
        exit(1);
    }



    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = ipaddr;

    //for know only sending data
    int ch = 1;
    struct timespec req = {.tv_sec = 0, .tv_nsec = 10*1000000};
    while( (ch = getchar()) > 0){

        sendto(sockfd, &ch, 1, 0, (const struct sockaddr*)&server_addr, socklen);
        nanosleep(&req, NULL);
    }

    return 0;
}