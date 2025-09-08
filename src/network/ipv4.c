#include <sys.h>
#include <pmm.h>
#include <fb.h>
#include <network/ipv4.h>
#include <network/arp.h>
#include <network/icmp.h>
#include <network/udp.h>
#include <network/tcp.h>

static uint16_t ip_ident_counter = 0;

void ipv4_handle(struct nic* dev, const struct eth_frame* eth, size_t len){

    int err;
    if(len < (sizeof(struct ipv4_packet) + sizeof(struct eth_frame)) ){
        fb_console_printf("ipv4: packet too small\n");
        return;
    }
    
    struct ipv4_packet* ip = (struct ipv4_packet*)eth->payload;    
    if(ip->version_ihl != 0x45){
        return;
    }

    //update the arp cache
    arp_cache_update(*(uint32_t*)ip->spa, eth->src_mac);


    //i only support icmp and udp for now
    switch(ip->protocol){
        case IPV4_PROTOCOL_ICMP:
        ;
        struct icmp_packet* icmp = (struct icmp_packet*)(((uint8_t*)ip) + (ip->version_ihl & 0xF)*4); 
        size_t icmp_length = ntohs(ip->total_length) - ((ip->version_ihl & 0xF)*4); //ICMP total size

        if(icmp->type == ICMP_TYPE_ECHO_REQUEST){ //if echo_req then echo back
            size_t total_length = icmp_length;
            total_length += sizeof(struct ipv4_packet);
            total_length += sizeof(struct eth_frame);

            //eth frame
            struct eth_frame* rframe = kcalloc(1, total_length);
            memcpy(rframe->dst_mac, eth->src_mac, 6);
            memcpy(rframe->src_mac, dev->mac, 6);
            rframe->ethertype = htons(ETHERFRAME_IPV4);

            //ipv4 frame
            struct ipv4_packet* ripv4 = (struct ipv4_packet*)rframe->payload;
            ripv4->version_ihl = 0x45;
            ripv4->TOS = 0;
            ripv4->total_length = htons(sizeof(struct ipv4_packet) + icmp_length);
            ripv4->identification = ip_ident_counter++;
            ripv4->flags_fragoffset = (0x02 << 5) | (0 <<3 ); //don't fragment
            ripv4->ttl = 64;
            ripv4->protocol = IPV4_PROTOCOL_ICMP;

            memcpy(ripv4->tpa, ip->spa, 4);
            memcpy(ripv4->spa, &dev->ip_addrs, 4);

            ripv4->header_checksum = 0;
            ripv4->header_checksum = compute_checksum((uint16_t*)ripv4, sizeof(struct ipv4_packet));
            
            //icmp frame
            struct icmp_packet* icmp_req = (struct icmp_packet*)((uint8_t*)ip + ((ip->version_ihl & 0xF)*4));
            struct icmp_packet* ricmp = (struct icmp_packet*)ripv4->payload;
            ricmp->type = ICMP_TYPE_ECHO_REPLY;
            ricmp->code = 0;
            ricmp->identifier = icmp_req->identifier;
            ricmp->sequence = icmp_req->sequence;
            memcpy(ricmp->data, icmp_req->data, icmp_length - sizeof(struct icmp_packet));
            ricmp->checksum = 0;
            ricmp->checksum = compute_checksum((uint16_t*)ricmp, icmp_length);

            dev->send(dev, rframe, total_length);
            kfree(rframe);

        }else{ //should look for raw sockets in

        }
        break;

        case IPV4_PROTOCOL_UDP:
        udp_net_send_socket(ip, ntohs(ip->total_length));
        break;

        case IPV4_PROTOCOL_TCP:
        err = tcp_net_send_socket(ip, ntohs(ip->total_length));
        if(err < 0){

        }
        break;


        
        case IPV4_PROTOCOL_IGMP:
        break;

        case IPV4_PROTOCOL_ENCAP:
        break;

        case IPV4_PROTOCOL_OSPF:
        break;

        case IPV4_PROTOCOL_SCTP:
        break;

    }

}