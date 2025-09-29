#include <network/icmp.h>
#include <network/ipv4.h>

int icmp_input(struct sk_buff* skb){
    struct icmp_packet* icmp = (struct icmp_packet*)skb->data;
    //ip addreses on skb are big endian!!!
    

    if(icmp->type == ICMP_TYPE_ECHO_REQUEST){
        
        skb_pull(skb, sizeof(struct icmp_packet));
        void* echo_payload =  skb->data;
        
        //allocate new skb for output and set metadata
        struct sk_buff* reply = skb_alloc(sizeof(struct icmp_packet) + skb->len + 64);
        
        //since it's reply, the source-destination is swapped ofc
        reply->src_ip = skb->dst_ip;
        reply->dst_ip = skb->src_ip;
        reply->protocol = IPV4_PROTOCOL_ICMP;
        reply->dev = skb->dev;
        
        
        skb_reserve(reply, 64); //reserve for eth and ip
        memcpy(skb_put(reply, skb->len), echo_payload, skb->len); //copy echo payload
        struct icmp_packet *ricmp = skb_push(reply, sizeof(struct icmp_packet));
        ricmp->type = ICMP_TYPE_ECHO_REPLY;
        ricmp->code = 0;
        ricmp->identifier = icmp->identifier;
        ricmp->sequence = icmp->sequence;
        ricmp->checksum = 0;
        ricmp->checksum = compute_checksum((uint16_t*)ricmp, reply->len);

        ip_output(reply);
    }


   return 0;
}


