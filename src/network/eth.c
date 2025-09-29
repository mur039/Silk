#include <network/eth.h>
#include <network/arp.h>
#include <network/ipv4.h>
#include <network/netif.h>
#include <network/skb.h>
#include <syscalls.h>


struct sk_buff* queued_eth_frames = NULL;

void eth_arp_callback(void* _e, void* arg){
    struct arp_entry* e = _e;
    struct sk_buff* skb = arg;
    if(e->state == ARP_FAILED){
        fb_console_printf("failed?");
        skb_free(skb);
        return;
    }   
    else if(e->state == ARP_RESOLVED){
        struct eth_frame *eth = (struct eth_frame*)skb->data;
        memcpy(eth->dst_mac, e->mac, 6);
        skb->dev->send(skb->dev, skb->data, skb->len);
        skb_free(skb);
        return;
    }   
    else if(e->state == ARP_INCOMPLETE){
        arp_send_request(skb->dev, skb->dst_ip);
        return;
    }   

}


int eth_output(struct nic *dev, struct sk_buff *skb) {
    
    int err = 0;
    // Push Ethernet header
    struct eth_frame *eth = skb_push(skb, sizeof(struct eth_frame));
    memcpy(eth->src_mac, dev->mac, 6);
    eth->ethertype = htons(ETHERFRAME_IPV4);
    
        
    if(skb->dst_ip == 0xffffffff){ //broadcast
        memset(eth->dst_mac, 0xff, 6);
    }
    else if( (skb->dst_ip & 0xF0000000) == 0xE0000000){ // 224.0.0.0 ... 239.255.255.255 //multicast
        eth->dst_mac[0] = 0x01;
        eth->dst_mac[1] = 0x00;
        eth->dst_mac[2] = 0x5e;
        eth->dst_mac[3] = ((skb->dst_ip >> 24) & 0xff) & 0x7F;  // lower 7 bits of second byte
        eth->dst_mac[4] = ((skb->dst_ip >> 16) & 0xff);
        eth->dst_mac[5] = ((skb->dst_ip >> 8) & 0xff);    
    
    }
    else{
        struct arp_entry* e = arp_cache_lookup(skb->dst_ip);
        if(!e){
            arp_query_request(skb->dst_ip, eth_arp_callback, skb);
            arp_send_request(skb->dev, skb->dst_ip);
            return 0;
        }

        //huh?
        // //the e might be failed so...
        // if(e->state != ARP_RESOLVED){
        //     skb_free(skb);
        //     return 0;
        // }
        
        memcpy(eth->dst_mac, e->mac, 6);
    }


    // Submit to NIC
    err = dev->send(dev, skb->data, skb->len);
    skb_free(skb);
    return err;
}



int eth_input(struct sk_buff* skb){
    /*
    * Ethernet requires on wire frames to be minimum 64 bytes with 4 byte CRC, which 
    * every package smaller than 60 bytes is well rounded to 60. don't count on sk_buff's length
    */
    struct eth_frame* eth = (struct eth_frame*)skb->data;
    
    uint16_t frametype = ntohs(eth->ethertype);
    
    switch(frametype){
        case ETHERFRAME_ARP: 
            skb_pull(skb, sizeof(struct eth_frame)); // data points to payload of eth
            arp_input(skb, eth);
        break;
        
        case ETHERFRAME_IPV4: 
            skb_pull(skb, sizeof(struct eth_frame));
            {
                struct ipv4_packet* ip = (struct ipv4_packet*)skb->data;
                arp_cache_update(*(uint32_t*)ip->spa, eth->src_mac);
            }
            ip_input(skb);
        break;

        // case ETHERFRAME_IPV6: break;
        default: //i don't know what is this so ignore it
            skb_free(skb);
        break;
    }
    

    return 0;
}