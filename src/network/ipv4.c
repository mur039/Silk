#include <sys.h>
#include <pmm.h>
#include <fb.h>
#include <network/ipv4.h>
#include <network/arp.h>
#include <network/icmp.h>
#include <network/udp.h>
#include <network/tcp.h>

#include <syscalls.h>
#include <network/route.h>

int ip_output(struct sk_buff* skb){


    if(!skb->dev){

        const route_t* r = route_select_route(skb->dst_ip);
        if(!r){
            return -ENOENT;
        }
        skb->dev = r->interface;
    }

    struct nic* dev = skb->dev;
    skb->src_ip = dev->ip_addrs;
    size_t ip_len = sizeof(struct ipv4_packet) + skb->len; 
    
    struct ipv4_packet *iph = skb_push(skb, sizeof(struct ipv4_packet));
    iph->version_ihl = 0x45;
    iph->TOS = 0;
    iph->total_length = htons(ip_len);
    iph->identification = htons(0);
    iph->flags_fragoffset =  (0x02 << 5) | (0 << 3);
    iph->ttl = 64;
    iph->protocol = skb->protocol;
    iph->header_checksum = 0;
    *(uint32_t*)iph->tpa = skb->dst_ip;
    *(uint32_t*)iph->spa = skb->src_ip;

    iph->header_checksum = compute_checksum((uint16_t *)iph, sizeof(struct ipv4_packet));
    skb->protocol = ETHERFRAME_IPV4;

    return dev->output(dev, skb);
}




int ip_input(struct sk_buff* skb){
    struct ipv4_packet* ip = (struct ipv4_packet*)skb->data;
    if(ip->version_ihl != 0x45){
        return 0;
    }

    //set metadata on skb structure
    skb->src_ip = *(uint32_t*)ip->spa;
    skb->dst_ip = *(uint32_t*)ip->tpa;

    switch (ip->protocol){
        case IPV4_PROTOCOL_ICMP: 
            skb_pull(skb, sizeof(struct ipv4_packet));
            icmp_input(skb);
        break;

        case IPV4_PROTOCOL_UDP: 
            udp_input(skb);//udp might need some ip thingy
        break;
        case IPV4_PROTOCOL_TCP: 
            tcp_input(skb); //as well as the tcp
        break;

        // case IPV4_PROTOCOL_IGMP: break;
        // case IPV4_PROTOCOL_ENCAP: break;
        // case IPV4_PROTOCOL_OSPF: break;
        // case IPV4_PROTOCOL_SCTP: break;
    
        default: break;
    }
    

    return 0;
}