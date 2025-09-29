#ifndef __ETH_H__
#define __ETH_H__


#include <stdint-gcc.h>
#include <network/skb.h>

struct eth_frame {
    uint8_t  dst_mac[6];
    uint8_t  src_mac[6];
    uint16_t ethertype;
    uint8_t  payload[];
} __attribute__((packed));

#define ETHERFRAME_IPV4  0x0800
#define ETHERFRAME_ARP  0x0806
#define ETHERFRAME_IPV6  0x86DD


int eth_output(struct nic *dev, struct sk_buff *skb);
int eth_input(struct sk_buff* skb);



#endif