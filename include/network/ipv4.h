#ifndef __IPV4_H__
#define __IPV4_H__

#include <stdint.h>
#include <sys.h>   
#include <network/netif.h> 




#define IPV4_PROTOCOL_ICMP 1
#define IPV4_PROTOCOL_IGMP 2
#define IPV4_PROTOCOL_TCP 6
#define IPV4_PROTOCOL_UDP 17
#define IPV4_PROTOCOL_ENCAP 41
#define IPV4_PROTOCOL_OSPF 89
#define IPV4_PROTOCOL_SCTP  132


struct ipv4_packet {
    uint8_t version_ihl; //version 4-bit, internet header length, number of 32-bits
    uint8_t TOS;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragoffset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint8_t spa[4]; // sender IP
    uint8_t tpa[4]; // target IP

    uint8_t payload[];
} __attribute__((packed));

void ipv4_handle(struct nic* dev, const struct eth_frame* eth, size_t len);




#endif
