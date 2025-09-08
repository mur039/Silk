#ifndef __INET_SOCKET_H__
#define __INET_SOCKET_H__

#include <stdint-gcc.h>
#include <socket.h>


struct inet_sock {
    struct sock sk;
    uint32_t bound_ip; //ipv4
    uint16_t bound_port;

    uint32_t connected_ip; //ipv4
    uint16_t connected_port;
};



struct socket* inet_socket_create(int type, int protocol);

//for ioctls
#define SIOCGIFCONF 1
#define SIOCGIFADDR 2  // get ip address
#define SIOCGIFHWADDR 3 //macaddr
#define SIOCSIFADDR 4 //set ip address
#define SIOCGIFNETMASK 5 //getnetmask 
#define SIOCSIFNETMASK 6 //set net mask

#endif