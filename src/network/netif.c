#include <network/netif.h>
#include <network/arp.h>
#include <network/ipv4.h>
#include <pmm.h>
#include <sys.h>
#include <fb.h>


#define MAX_NETIF_COUNT 8
struct nic netifs[MAX_NETIF_COUNT];

static struct nic* free_nic;
static struct nic* alloc_nic;

void initialize_netif(void){
    memset(netifs, 0, sizeof(netifs));
    
    //create a linked list of free nics
    alloc_nic = NULL;
    free_nic = netifs;
    struct nic* head = free_nic;
    for(int i = 0; i < MAX_NETIF_COUNT - 1; ++i){
            head = &netifs[i];
            head->next = &netifs[i + 1];
            head = head->next;
    }

    timer_register(333, 333, arp_periodic_check, (void*)1);
    
    //initialize the lo?
    // struct nic* lo = netif_allocate();
    // strcpy(lo->name, "lo");
    // lo->mtu = 1500;
    // lo->ip_addrs = 0x7F000001;
    // lo->gateway = 0x7F000001;
    // memset(lo->mac, 0, 6);
    
    // lo->next = NULL;
    // lo->send = NULL;
    // lo->handle_rx = NULL;
    // lo->poll = NULL;
    // lo->priv = NULL;
    
}

struct nic* netif_allocate(){

    //no nic left
    if(!free_nic) 
        return NULL;

    //get free nic
    struct nic* candidate = free_nic;
    free_nic = free_nic->next;

    //mark it allocated
    candidate->next = alloc_nic;
    alloc_nic = candidate;

    return candidate;
}


int net_should_frame_receive( const struct nic* dev, const struct eth_frame* frame){
    
    if( !memcmp(dev->mac, frame->dst_mac, 6) ) //our mac
        return 1;
    
    const uint8_t broadcast_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if( !memcmp(broadcast_mac, frame->dst_mac, 6) ) //broadcast mac
        return 1;

    if( frame->dst_mac[0] & 1) //multicast
        return 1; // there's also something called multicast groups?
    
    return 0;
}


//all networking entry point
void net_receive(struct nic *dev, const void *_frame, size_t len){

    // fb_console_printf("from nic %s, i received: ", dev->name);
    struct eth_frame* frame = (struct eth_frame*)_frame;

    //check the macaddr
    //if not our mac, ethernet broadcast or multicast reject it
    if( !net_should_frame_receive(dev, frame) ){ 
        return;
    }

    uint16_t type = ntohs(frame->ethertype);

    switch(type){
        case ETHERFRAME_ARP:
            arp_handle(dev, frame, len);
        break;

        case ETHERFRAME_IPV4:
            ipv4_handle(dev, frame, len);
        break;
        
        case ETHERFRAME_IPV6: break; //we don't support ipv6

    }
    return;
}



//by the RFC1071
uint16_t compute_checksum(uint16_t *data, size_t len) { 
    uint32_t sum = 0;
    
    while (len > 1) {
        sum += *data++;
        len -= 2;
    }
    if (len == 1) {
        sum += *((uint8_t*)data);
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

int netif_ioctl_list_interfaces(struct ifconf* conf){
    
    //get total number of nics
    size_t no_nics = 0;
    for(struct nic* head = alloc_nic; head; head = head->next) no_nics++;
    
    if((int)(sizeof(struct ifreq) * no_nics) > conf->ifc_len)
        return -1;
    
    int bytes_written = 0;

    int index = 0;
    struct ifreq* req = conf->ifc_req;
    for(struct nic* head = alloc_nic; head; head = head->next){
        
        memset(req, 0, sizeof(struct ifreq));
        memcpy(req->ifr_name, head->name, IFNAMSIZ);
        req->ifr_addr = head->ip_addrs;
        req->ifr_ifindex = index++;
        
        req += 1;
        bytes_written += sizeof(struct ifreq);
        if(bytes_written > conf->ifc_len) break;        
    }

    conf->ifc_len = bytes_written;

    return 0;
}

struct nic* netif_get_by_name( const char* name){

    for(struct nic* head = alloc_nic; head; head = head->next){
        
        if(!strcmp(head->name, name) ){
            return head;
        }
     
    }

    return NULL;
}