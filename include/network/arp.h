#ifndef __ARP_H__
#define __ARP_H__

#include <stdint-gcc.h>
#include <stddef.h>
#include <network/netif.h>

#define ARP_REQUEST 1
#define ARP_REPLY   2

struct arp_packet {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6]; // sender MAC
    uint8_t spa[4]; // sender IP
    uint8_t tha[6]; // target MAC
    uint8_t tpa[4]; // target IP
} __attribute__((packed));



#define MAC_ADDR_LEN 6
#define MAX_ARP_WAITERS 8 
enum arp_entry_state {
    ARP_INCOMPLETE, 
    ARP_RESOLVED,   
    ARP_FAILED,
    ARP_STATE_END
};

struct arp_waiter {
    void (*callback)(void *arg); // function to resume (e.g., send packet)
    void *arg;                   // context (e.g., packet, socket, etc.)
};

struct arp_entry {
    struct arp_entry *next;     // For hash table chaining
    uint32_t ip_addr;           // IPv4 in network byte order
    uint8_t mac[MAC_ADDR_LEN];  // Valid if state == RESOLVED

    enum arp_entry_state state;
    uint64_t timestamp;         // Last updated (for aging)
    int retries;                // For retrying requests
    struct arp_waiter waiters[MAX_ARP_WAITERS];
    int num_waiters;
};

#define ARP_TABLE_SIZE 256 
struct arp_cache {
    struct arp_entry *buckets[ARP_TABLE_SIZE];
};



void arp_handle(struct nic* dev, const struct eth_frame* eth, size_t len);
struct arp_entry* arp_cache_lookup(uint32_t ipaddr);
void arp_cache_update( uint32_t ipaddr, const uint8_t macaddr[6]);
void arp_query_request(uint32_t ipaddr, void (*callback_func)(void*), void* callbackarg );
void arp_periodic_check(void*);
void arp_send_request(struct nic* dev, uint32_t ipaddr);
#endif