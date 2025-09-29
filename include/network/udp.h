#ifndef __UDP_H__
#define __UDP_H__

#include <stdint-gcc.h>
#include <network/ipv4.h>
#include <network/inet_socket.h>
struct udp {
    uint16_t sport;
    uint16_t dport;
    uint16_t length;
    uint16_t checksum;
    uint8_t data[];
} __attribute__((packed));


struct udp_sock{
    struct inet_sock isk;
    
};


int udp_input(struct sk_buff* skb);

int udp_create_sock(struct socket* socket);
uint16_t udp_calculate_checksum(struct udp* udp, uint8_t* destip, uint8_t* senderip);
int udp_get_ephemeral_port();

#endif