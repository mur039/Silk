#ifndef __NETIF_H__
#define __NETIF_H__

#include <sys.h>
#include <network/eth.h>
#include <socket.h>


struct nic {
    char name[16];                   
    uint8_t mac[6];                  
    uint32_t mtu; // Maximum transmission unit
    uint32_t ip_addrs;  // Assigned IPv4 address
    uint32_t gateway;  // Assigned gateaway


    int (*send)(struct nic *dev, const void *packet, size_t len);
    int (*poll)(struct nic *dev);   // Optional: to pull incoming packets (used in polling model)
    void (*handle_rx)(struct nic *dev, const void *data, size_t len);
    int (*output)(struct nic* dev, struct sk_buff* skb);
    
    void *priv;                     // Driver-specific data (e.g. e1000 ring buffers)
    
    struct nic *next;               // For chaining NICs (if you want a global NIC list)
};



void initialize_netif(void);
struct nic* netif_allocate();
void net_receive(struct nic *dev, const void *frame, size_t len);

uint16_t compute_checksum(uint16_t *data, size_t len);

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

int netif_ioctl_list_interfaces(struct ifconf* conf);
struct nic* netif_get_by_name( const char* name);



#endif