#include <network/tcp.h>

static uint32_t tcp_port_bitmap[2048];

static int tcp_is_port_bound(int port){
    
    if(port < 0 || port > 65535 ){
        return -1;
    }

    int word_i = port / 32;
    int bit_i = port % 32;

    return GET_BIT( tcp_port_bitmap[word_i], bit_i );
}

int tcp_net_send_socket(const struct ipv4_packet* ip, size_t len){

    struct tcp* tcp = (struct tcp *)((uint8_t*)ip + (ip->version_ihl & 0xF)*0x4);

    sizeof(struct tcp);
    int drc = ntohs(tcp->data_off_rsrvd_ctrl);
    int data_offset =  (drc  >> 12) & 0xF;
    uint8_t ctrl = drc &  0x3F;

    fb_console_printf(
        "Source port: %u\n"
        "Destination port: %u\n"
        "Sequence number: %x\n"
        "Ack number: %u\n"
        "data offset: %u\n",
        
        ntohs(tcp->source_port) ,
        ntohs(tcp->destination_port),
        
        ntohl(tcp->seq_numb),
        ntohl(tcp->ack_numb),
        data_offset
    );

    for(int i = 0; i < 6; ++i){
        
        if( !GET_BIT(ctrl, i)  ) continue;

        
        switch(1 << i){
        case URG: fb_console_put("URG "); break;;
        case ACK: fb_console_put("ACK "); break;;
        case PSH: fb_console_put("PSH "); break;;
        case RST: fb_console_put("RST "); break;;
        case SYN: fb_console_put("SYN "); break;;
        case FIN: fb_console_put("FIN "); break;;
        }
        
    }

    fb_console_putchar('\n');
    fb_console_printf(
        "window: %u\n"
        "checksum: %x\n"   ,
        
        ntohs(tcp->window),
        ntohs(tcp->checksum)
    );

    size_t payload_size = len;
    payload_size -= (ip->version_ihl & 0xF)*0x4; // ipv4 header
    fb_console_printf("data offset values: %u and mul by 4 :%u\n", data_offset, data_offset * 4);

    //in the structure
    fb_console_printf("In the structure data is at offset: %u \n",  (size_t)((uint8_t*)tcp->data - (uint8_t*)tcp) );
    payload_size -= data_offset*4;
    uint8_t* payload = ((uint8_t*)tcp + (data_offset)*0x4);
    if(payload_size != 0){
        
        fb_console_printf("With payload size: %u:\n", payload_size);
        kxxd(payload, payload_size);
    }
    
    //no one is bound at that port for tcp
    if( !tcp_is_port_bound(ntohs(tcp->destination_port)) ){
        
        return -1; //upper layer have more information
    }
    

}

uint16_t tcp_package_calc_checksum(struct tcp* package, size_t len, uint32_t src_ip, uint32_t dst_ip) {

    struct pseudo_header {
        uint32_t src_ip;
        uint32_t dst_ip;
        uint8_t  zero;
        uint8_t  protocol;
        uint16_t tcp_len;
    } __attribute__((packed));

    struct pseudo_header ph;
    ph.src_ip = src_ip;//htonl(src_ip);
    ph.dst_ip = dst_ip;//htonl(dst_ip);
    ph.zero = 0;
    ph.protocol = 6; // TCP
    ph.tcp_len = htons(len);

    // Calculate checksum over pseudo-header + tcp segment
    uint8_t buf[512]; // max TCP segment size, adjust as needed
    size_t total_len = sizeof(ph) + len;

    if (total_len > sizeof(buf)) return 0; // too big

    memcpy(buf, &ph, sizeof(ph));
    memcpy(buf + sizeof(ph), package, len);

    return  compute_checksum((uint16_t*)buf, total_len);

}