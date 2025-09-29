#ifndef __SKB_H__
#define __SKB_H__

#include <stdint-gcc.h>
#include <socket.h>

struct sk_buff {
    uint8_t *head;   // start of allocated memory
    uint8_t *data;   // current packet start (moves as headers are pushed/popped)
    uint8_t *tail;   // end of current packet
    uint8_t *end;    // end of allocated memory

    size_t len;      // length of valid data

    struct nic *dev;  // owning interface
    uint32_t src_ip, dst_ip;
    uint16_t protocol;  // ETH_P_IP, ETH_P_ARP, etc.

    struct sk_buff *next; // for queues
};

struct sk_buff *skb_alloc(size_t size);
void skb_free(struct sk_buff *skb);
void skb_reserve(struct sk_buff *skb, size_t len);
void *skb_put(struct sk_buff *skb, size_t len);
void *skb_push(struct sk_buff *skb, size_t len);
void *skb_pull(struct sk_buff *skb, size_t len);
void skb_trim(struct sk_buff *skb, size_t new_len);



#endif