#ifndef     __ICMP_H__
#define     __ICMP_H__

#include <stdint-gcc.h>
#include <network/skb.h>

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_REQUEST 8

struct icmp_packet {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
    uint8_t data[];
} __attribute__((packed));

int icmp_input(struct sk_buff* skb);

#endif
