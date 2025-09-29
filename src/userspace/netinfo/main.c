#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

#include <sys/socket.h>
#include <stdint.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

int if_get_interfaces(int sockfd ,struct ifconf* ifc){
    
    if( ioctl(sockfd, SIOCGIFCONF, ifc) == -1){
        printf("[-]failed to ioctl");
        return -1;
    }

    struct ifreq* ifr = ifc->ifc_req;
    int n_interfaces = ifc->ifc_len / sizeof(struct ifreq);

    return n_interfaces;
}


uint8_t buffer[512]; //general purpose buffer

int main( int argc, char* argv[]){


    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0){
        puts("Failed to allocate a socket\n");
        return 1;
    }    

    struct ifconf ifc = {.ifc_len = 512, .ifc_buf = buffer};
    int ifcount = if_get_interfaces(fd, &ifc);
    struct ifreq* ifr = ifc.ifc_req;

    sizeof(struct ifreq);
    for(int i = 0; i < ifcount; ++i){
        
        ioctl(fd, SIOCGIFADDR, &ifr[i]); //get ip address
        char* if_name = ifr[i].ifr_name;

        uint32_t if_addr = ifr[i].ifr_addr;
        printf("%s: address:%u.%u.%u.%u\n", 
            ifr[i].ifr_name,
            (ifr[i].ifr_addr >> 0) & 0xff,
            (ifr[i].ifr_addr >> 8) & 0xff,
            (ifr[i].ifr_addr >> 16) & 0xff,
            (ifr[i].ifr_addr >> 24) & 0xff
        );
    }

    return 0;
}

