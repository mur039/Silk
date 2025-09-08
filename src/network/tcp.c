#include <network/inet_socket.h>
#include <network/tcp.h>
#include <syscalls.h>

static uint32_t tcp_port_bitmap[2048];
static list_t tcp_bound_sockets = {.head = NULL, .tail = NULL, .size = 0};


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





//proto_ops
int tcp_proto_ops_bind(file_t* file, struct sockaddr* addr, socklen_t len){

    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct tcp_sock *tsk = socket->protocol_data;
    struct inet_sock* isk = &tsk->isk;

    struct sockaddr_in* iaddr = (struct sockaddr_in*)addr;
    if(socket->state != SS_UNCONNECTED){
        return -EINVAL;
    }

    uint32_t ip_addr = ntohl(iaddr->sin_addr);
    uint32_t port = ntohs(iaddr->sin_port);
    
    //check the port
    int word_i = port / 32;
    int bit_i = port % 32;
    if(GET_BIT(tcp_port_bitmap[word_i], bit_i)){ //port already boun
        return -EADDRINUSE;
    }

    tcp_port_bitmap[word_i] |=  1 << bit_i;
    isk->bound_ip = ip_addr;
    isk->bound_port = port;

    socket->iconn = (void*)list_insert_end(&tcp_bound_sockets, socket); //wonky ass shit
    
    return 0;
}

int tcp_proto_ops_accept(file_t* file, struct sockaddr* addr, socklen_t* len){
    return -EINVAL;
}

int tcp_proto_ops_connect(file_t* file, struct sockaddr* addr, socklen_t len){
    return -EINVAL;
}

int tcp_proto_ops_sendmsg(file_t* file, void* addr, size_t len){
    return -EINVAL;
}

int tcp_proto_ops_recvmsg(file_t *file, struct msghdr* msg, int flags){
    return -EINVAL;
}

int tcp_proto_ops_sendto(file_t* file, void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen){
    return -EINVAL;
}




struct proto_ops tcp_proto_ops = {
    .bind = (void*)tcp_proto_ops_bind,
    .accept = (void*)tcp_proto_ops_accept,
    .connect = (void*)tcp_proto_ops_connect,
    .sendmsg = (void*)tcp_proto_ops_sendmsg,
    .recvmsg = (void*)tcp_proto_ops_recvmsg,
    .sendto = (void*)tcp_proto_ops_sendto
};



//proto
int tcp_proto_disconnect(struct sock *sk, int flags){
    return -EINVAL;
}

void tcp_proto_destroy(struct sock *sk){
    return;
}

struct proto tcp_proto = {
    .disconnect = tcp_proto_disconnect,
    .destroy = tcp_proto_destroy,
};



int tcp_create_socket(struct socket* socket){

    struct tcp_sock *tsk = kcalloc(1, sizeof(struct inet_sock));
    if(!tsk){
        return -ENOMEM;
    }

    struct sock* sk = &tsk->isk.sk;
    sk->sk_refcount++;
    sk->sk_socket = socket;
    sk->sk_proc = current_process->pid;

    sk->recv = circular_buffer_create(4096);
    sk->pending_reads = list_create();
    sk->pending_writes = list_create();

    socket->protocol_data = sk;
    
    sk->sk_prot = &tcp_proto;
    socket->ops = &tcp_proto_ops;
    
    return 0;
}