#ifndef __SYS_SOCKET_H__
#define __SYS_SOCKET_H__

#include <stddef.h>
#include <sys/syscall.h>
#include <stdint-gcc.h>
//domains
#define AF_UNIX 0
#define AF_LOCAL AF_UNIX
#define AF_INET 1
#define AF_NETLINK 8

//socket types
#define SOCK_STREAM 0
#define SOCK_DGRAM 1 
#define SOCK_RAW 3



typedef unsigned int sa_family_t;
typedef size_t socklen_t;
struct sockaddr {
    sa_family_t     sa_family;      /* Address family */
    char            sa_data[];      /* Socket address */
};

struct sockaddr_un {
    sa_family_t sun_family;               /* AF_UNIX */
    char        sun_path[108];            /* Pathname */
};

struct sockaddr_in {
    sa_family_t sin_family;               /* AF_UNIX */
    uint16_t sin_port;
    uint32_t sin_addr;
};

static inline uint16_t ntohs(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint16_t htons(uint16_t x){
    return (x << 8) | (x >> 8) ;

}


static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FFU) << 24) |
           ((hostlong & 0x0000FF00U) << 8)  |
           ((hostlong & 0x00FF0000U) >> 8)  |
           ((hostlong & 0xFF000000U) >> 24);
}

static inline uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong); // same operation
}




struct ifconf {
    int ifc_len;                // Size of buffer in bytes
    union {
        char *ifc_buf;         // Buffer to receive struct ifreq[]
        struct ifreq *ifc_req; // Array of ifreq structures
    };
};

#define IFNAMSIZ 16

struct ifreq {
    char ifr_name[IFNAMSIZ];  // Interface name, e.g., "eth0"
    union {
        uint32_t ifr_addr;   // Address (IP)
        uint32_t ifr_dstaddr;// P2P destination address
        uint32_t ifr_broadaddr; // Broadcast address
        uint32_t ifr_netmask;  // Netmask
        struct sockaddr ifr_hwaddr;     // MAC address (SIOCGIFHWADDR)
        short           ifr_flags;    // Interface flags
        int             ifr_ifindex;  // Interface index
        
        struct ifmap {
            unsigned long mem_start;
            unsigned long mem_end;
            unsigned short base_addr;
            unsigned char irq;
            unsigned char dma;
            unsigned char port;
        } ifr_map;

    };
};

#define SIOCGIFCONF 1
#define SIOCGIFADDR 2  //ip address
#define SIOCGIFHWADDR 3 //macaddr
#define SIOCSIFADDR 4 //set ip address


int socket(int domain, int type, int protocol);
int listen(int sockfd, int backlog);
int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int accept(int sockfd, struct sockaddr* addr, socklen_t* len);
int sendto(int sockfd, void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen);


#define SOL_SOCKET 0
#define SO_BINDTODEVICE 1
int setsockopt(int sockfd, int level, int optname,const void* optval, socklen_t optlen);
size_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr * restrict src_addr, socklen_t * restrict addrlen);

#endif