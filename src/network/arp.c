#include <network/arp.h>
#include <network/netif.h>
#include <sys.h>
#include <fb.h>



static struct arp_cache cache;

static const char* arp_state_strings[] = {
    [ARP_INCOMPLETE] = "ARP_INCOMPLETE",
    [ARP_RESOLVED] = "ARP_RESOLVED",
    [ARP_FAILED] = "ARP_FAILED",
    [ARP_STATE_END] = "ARP_UNKNOWN_STATE",
    NULL
};

static inline int arp_hash(uint32_t ip) {
    return ip & (ARP_TABLE_SIZE - 1); // assuming power-of-2 size
}


struct arp_entry* arp_cache_lookup(uint32_t ipaddr){
    uint32_t hash = arp_hash(ipaddr);
    struct arp_entry *e = cache.buckets[hash];
    
    while (e){
        if (e->ip_addr == ipaddr)
            return e;
        e = e->next;
    }
    return NULL;
}

void arp_cache_update( uint32_t ipaddr, const uint8_t macaddr[6]){

    struct arp_entry *e = arp_cache_lookup(ipaddr);
    
    if(e){ //an entry exist whether its incomplete or resolved, we mark it resolved and write mac address
        memcpy(e->mac, macaddr, 6);
        e->state = ARP_RESOLVED;
        e->timestamp = get_ticks();
        e->retries = 0;

        //somehow wakeup waiters?
        while(e->num_waiters > 0){
            e->num_waiters--;
            if(e->waiters[e->num_waiters].callback)
                e->waiters[e->num_waiters].callback(e);
        }
    }
    else{ //does not exist so

        e = kcalloc(sizeof(struct arp_entry), 1);
        e->ip_addr = ipaddr;
        memcpy(e->mac, macaddr, 6);
        e->state = ARP_RESOLVED;
        e->timestamp = get_ticks();
        uint32_t hash = arp_hash(ipaddr);
        e->next = cache.buckets[hash];
        cache.buckets[hash] = e;

    }
}


//called by timer subsystem
void arp_periodic_check(void* _unused){
    for(int i = 0; i < ARP_TABLE_SIZE; ++i){
        struct arp_entry* e = cache.buckets[i];
        if(!e) continue; //empty entry

        //we check for hashes then there's hash chaining...
        int j = 0;
        while(e){

            if(e->state == ARP_INCOMPLETE){

                e->retries--;
                if(e->retries == 0 ){ //fails
                    e->state = ARP_FAILED; 
                    //somehow clean up?
                    //make caller know aobut the failed arp?
                    while(e->num_waiters > 0){
                        e->num_waiters--;
                        if(e->waiters[e->num_waiters].callback)
                            e->waiters[e->num_waiters].callback(e);
                    }
                    
                }

            }
            else if((unsigned long)e->state > ARP_STATE_END){
                //should remove invalid state somehow?
                fb_console_printf("!!!!INVALID ARP STATE: %u\n", e->state);
            }
            
            
            j++;
            e = e->next;
     }
    }
}





static void send_arp_reply(struct nic* dev, uint8_t *target_mac, uint8_t *target_ip) {
    struct eth_frame {
        uint8_t dst_mac[6];
            uint8_t src_mac[6];
        uint16_t ethertype;
        struct arp_packet arp;
    } __attribute__((packed)) frame;

    memcpy(frame.dst_mac, target_mac, 6);
    memcpy(frame.src_mac, dev->mac, 6);
    frame.ethertype = htons(ETHERFRAME_ARP); // ARP

    frame.arp.htype = htons(1);       // Ethernet
    frame.arp.ptype = htons(0x0800);  // IPv4
    frame.arp.hlen = 6;
    frame.arp.plen = 4;
    frame.arp.oper = htons(2);        // Reply

    memcpy(frame.arp.sha, dev->mac, 6);
    memcpy(frame.arp.spa, &dev->ip_addrs, 4);
    memcpy(frame.arp.tha, target_mac, 6);
    memcpy(frame.arp.tpa, target_ip, 4);

    int index = dev->send(dev, &frame, sizeof(frame));
    return;
}


void arp_handle(struct nic* dev, const struct eth_frame* eth, size_t len){

    struct arp_packet *arp = (struct arp_packet *)eth->payload;
    uint16_t htype, ptype, oper;
    htype = ntohs(arp->htype);
    ptype = ntohs(arp->ptype);
    oper = ntohs(arp->oper);
    
    switch (oper){
        case ARP_REQUEST:   
        
            //check if it is nic's ip address
            if(memcmp(&dev->ip_addrs, arp->tpa, 4)) return; //not our ip
            send_arp_reply(dev, arp->sha, arp->spa);
        break;

        case ARP_REPLY:
            arp_cache_update( ntohl(*(uint32_t*)arp->spa), eth->src_mac);
        break;
    }

}

void arp_query_request(uint32_t ipaddr, void (*callback_func)(void*), void* callbackarg ){
    
    struct arp_entry* e = kcalloc(sizeof(struct arp_entry), 1);
    e->ip_addr = ipaddr;
    
    e->state = ARP_INCOMPLETE;
    e->timestamp = get_ticks();
    e->retries = 7;
    e->waiters[e->num_waiters].callback = callback_func;
    e->waiters[e->num_waiters].arg = callbackarg;
    e->num_waiters++;
    
    uint32_t hash = arp_hash(ipaddr);
    e->next = cache.buckets[hash];
    cache.buckets[hash] = e;


}


void arp_send_request(struct nic* dev, uint32_t ipaddr){

    struct eth_frame {
        uint8_t dst_mac[6];
            uint8_t src_mac[6];
        uint16_t ethertype;
        struct arp_packet arp;
    } __attribute__((packed)) frame;

    memset(frame.dst_mac, 0xff, 6); //arp broadcast
    memcpy(frame.src_mac, dev->mac, 6);
    frame.ethertype = htons(ETHERFRAME_ARP); // ARP

    frame.arp.htype = htons(1);       // Ethernet
    frame.arp.ptype = htons(0x0800);  // IPv4
    frame.arp.hlen = 6;
    frame.arp.plen = 4;
    frame.arp.oper = htons(ARP_REQUEST);        // Reply

    memcpy(frame.arp.sha, dev->mac, 6);
    memcpy(frame.arp.spa, &dev->ip_addrs, 4);
    memset(frame.arp.tha, 0, 6); //unknown mac
    ipaddr = htonl(ipaddr);
    memcpy(frame.arp.tpa, &ipaddr, 4);

    int index = dev->send(dev, &frame, sizeof(frame));
    return;

}