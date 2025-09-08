#include "network/udp.h"
#include "network/netif.h"
#include "network/ipv4.h"
#include "network/arp.h"
#include "network/route.h"
#include <socket.h>
#include <syscalls.h>
#include "pmm.h"
#include "fb.h"


static uint32_t udp_port_bitmap[2048];
static list_t bound_sockets = {.head = NULL, .tail = NULL, .size = 0};

struct udp_recv{
    struct sockaddr_in source;
    uint16_t payload_len;
    uint8_t payload[];
};


#include <network/arp.h>
static void udp_arp_reply_callback(void* value){
    //this is called by both periodic checker and arp_cache_update
    // how can i check it tho?
    //sending the arp_entry???
    struct arp_entry* e = value;
    struct arp_waiter* w = e->waiters;

    struct socket* socket = w[e->num_waiters].arg;
    struct udp_sock* usk = socket->protocol_data;

    if(e->state == ARP_FAILED){ //probably from periodic check
        //maybe remove it from outgoing list?
        listnode_t* n = list_pop_end(&usk->isk.sk.pending_writes);
        if(n){

            kfree(n->val);
            kfree(n);
        }
        return;
    }
    //probably from arp_update_cache
    
    struct nic* dev = route_select_interface(e->ip_addr);

    listnode_t* n = list_pop_end(&usk->isk.sk.pending_writes);
    if(!n)  //no queued data?
        return;
    uint8_t* frame = n->val;
    kfree(n);

    //fill the eth frame
    struct eth_frame* eth = (void*)frame;
    memcpy(eth->dst_mac, e->mac, 6); //for know we know target mac

    size_t len = sizeof(struct eth_frame) + sizeof(struct ipv4_packet);
    struct udp* udp = (void*)&frame[len];
    len += ntohs(udp->length);
    
    dev->send(dev, frame, len);
    kfree(frame);

}


int udp_get_ephemeral_port(){

    uint32_t* bitmap = udp_port_bitmap;
    for(uint16_t i = EPHEMERAL_PORT_START; i < EPHEMERAL_PORT_END; ++i){
        int wordi = i / 32;
        int biti = i % 32;

        if( bitmap[wordi] == 0xFFFFFFFF) continue; //non-empty word
        if( !GET_BIT( bitmap[wordi], biti) ){
            //empty port
            return i;
        } 

    }
    return -1;
}

int udp_proto_ops_bind(file_t* file, struct sockaddr* addr, socklen_t len){
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct udp_sock* usk = socket->protocol_data;

    struct sockaddr_in* iaddr = (struct sockaddr_in*)addr;
    
    if(socket->state != SS_UNCONNECTED){
        return -EINVAL;
    }
    
    uint32_t ip_addr = ntohl(iaddr->sin_addr);
    uint32_t port = ntohs(iaddr->sin_port);
    //check the port

    int word_i = port / 32;
    int bit_i = port % 32;
    if(GET_BIT(udp_port_bitmap[word_i], bit_i)){ //port already boun
        return -EADDRINUSE;
    }

    //mark it as bound
    udp_port_bitmap[word_i] |=  1 << bit_i;
    usk->isk.bound_port = port;
    usk->isk.bound_ip = ip_addr;
    
    socket->state = SS_LISTENING;
    socket->iconn = (void*)list_insert_end(&bound_sockets, socket); //wonky ass shit
    return 0;
}

int udp_proto_ops_accept(file_t* file, struct sockaddr* addr, socklen_t* len){
    
    //udp is not acceptable
    return -EINVAL;
}

int udp_proto_ops_connect(file_t* file, const struct sockaddr* addr, socklen_t len){
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct udp_sock* usk = socket->protocol_data;

    struct sockaddr_in* iaddr = (struct sockaddr_in*)addr;
    usk->isk.bound_ip = ntohl(iaddr->sin_addr);
    usk->isk.bound_port = ntohs(iaddr->sin_port);

    socket->state = SS_CONNECTED;
    return 0;
}

int udp_proto_ops_sendto(file_t* file, void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen){
    
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct udp_sock* usk = socket->protocol_data;

    struct sockaddr_in* iaddr  = (struct sockaddr_in*)addr;
    
    uint32_t ipv4_addr = ntohl(iaddr->sin_addr);
    uint16_t port = ntohs(iaddr->sin_port);
    uint8_t* ipaddress = (uint8_t*)&ipv4_addr;

    
    struct nic* dev = NULL;
    
    uint8_t target_mac[6];
    if( (ipv4_addr & 0xF0000000) == 0xE0000000){ // 224.0.0.0 ... 239.255.255.255 //multicast

        //we must be bound
        if(!socket->bound_if){ //not bounded
            fb_console_printf("Not bound to an interface\n");
            return -EINVAL;
        }

        dev = socket->bound_if;

        target_mac[0] = 0x01;
        target_mac[1] = 0x00;
        target_mac[2] = 0x5e;
        target_mac[3] = ipaddress[1] & 0x7F;  // lower 7 bits of second byte
        target_mac[4] = ipaddress[2];
        target_mac[5] = ipaddress[3];
    }
    else if( ipv4_addr == 0xFFFFFFFF){ //broadcast
        
        //we must be bound
        if(!socket->bound_if){ //not bounded
            fb_console_printf("Not bound to an interface\n");
            return -EINVAL;
        }

        dev = socket->bound_if;

        memset(target_mac, 0xFF, 6);
    }
    else{ //  a specific ip address so arp resolution must be done

        
        const route_t* r = route_select_route(ipv4_addr);
        
        if(!r){
            fb_console_printf("Failed to find route?\n");
            return -EINVAL;
        }
        
        dev = r->interface;
        
        struct arp_entry* e;
        if(r->gateway){ //indirectly reachable
            e =  arp_cache_lookup(htonl(r->gateway));
        }else{ //directly reachable
            e =  arp_cache_lookup(htonl(ipv4_addr));
        }
        

        if(!e){
            memset(target_mac, 0, 6); //for below
        }
        else{
            memcpy(target_mac, e->mac, 6);
        }
        
    }

    //bind epi
    if(!usk->isk.bound_port){
        struct sockaddr_in ead;
        int port = udp_get_ephemeral_port();
        if(port < 0){
            return -EADDRINUSE;
        }

        ead.sin_family = AF_INET;
        ead.sin_port = htons(port);
        ead.sin_addr = htonl(dev->ip_addrs);
        int err = udp_proto_ops_bind(file, (struct sockaddr*)&ead, sizeof(ead) );
        if(err < 0) return err;
        
    }

    size_t total_size = sizeof(struct eth_frame) +sizeof(struct ipv4_packet) + sizeof(struct udp) + len;

    if(total_size > dev->mtu){
        return -EINVAL;
    }

    //create the packet then;
    uint8_t* raw_data = kcalloc(1, total_size);
    struct eth_frame* eth = (struct eth_frame*)raw_data;
    struct ipv4_packet* ip4 = (struct ipv4_packet*)eth->payload;
    struct udp* udp =  (struct udp*)ip4->payload;

    //from bottom to the top
    memcpy(udp->data, buf, len);
    udp->checksum = 0;
    udp->length = htons( sizeof(struct udp) + len );
    udp->sport = htons(usk->isk.bound_port);
    udp->dport = htons(port);

    uint32_t dstip = htonl(*(uint32_t*)ipaddress);
    uint32_t senderip = (dev->ip_addrs);
    udp_calculate_checksum(udp, (uint8_t*)&dstip, (uint8_t*)&senderip );

    ip4->version_ihl = 0x45;
    ip4->TOS = 0;
    ip4->total_length = htons(sizeof(struct ipv4_packet) + sizeof(struct udp) + len );
    ip4->identification = htons(0);
    ip4->flags_fragoffset =  (0x02 << 5) | (0 << 3);
    ip4->ttl = 64;
    ip4->protocol = IPV4_PROTOCOL_UDP;
    ip4->header_checksum = 0;

    memcpy(ip4->tpa, ipaddress, 4);
    memcpy(ip4->spa, &dev->ip_addrs, 4);

    //this needs to be be
    uint32_t* wordptr = (uint32_t*)ip4->tpa;
    *wordptr = htonl(*wordptr);

    wordptr = (uint32_t*)ip4->spa;
    // *wordptr = htonl(*wordptr);

    ip4->header_checksum = compute_checksum((uint16_t *)ip4, sizeof(struct ipv4_packet));

    eth->ethertype = htons(ETHERFRAME_IPV4);
    memcpy(eth->src_mac, dev->mac, 6);
    memcpy(eth->dst_mac, target_mac, 6); //for know we know target mac

    if(!memcmp(target_mac, "\0\0\0\0\0\0", 6)){
        //arp lookup failed so we write it to write_buffer of this socket, then
        //query an arp request to the network, when we got a request we send the packet with correct
        //mac addr. For udp we don't need to notify user about unreachable host
        list_insert_end(&usk->isk.sk.pending_writes, raw_data);
        arp_query_request(ipv4_addr,  udp_arp_reply_callback, socket);
        arp_send_request(dev, ipv4_addr);
        return 0;
    }

    dev->send(dev, raw_data, total_size);
    kfree(raw_data);
    

    return 0;
}
    


int udp_proto_ops_sendmsg(file_t* file, void* addr, size_t len){
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct udp_sock* usk = socket->protocol_data;

    

    return 0;
}


int udp_proto_ops_recvmsg(file_t *file, struct msghdr* msg, int flags){

    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct udp_sock* usk = socket->protocol_data;

    int min_size = sizeof(struct udp_recv); //least 2 bytes?
    int remaining = circular_buffer_avaliable(&usk->isk.sk.recv);
    
    //okay so sockaddr_in, length and 1 byte payload min remaining must be 12 + 2 + 1 = 15 bytes
    if(remaining < min_size){ //not enough data thus
        return -EAGAIN;
    }
    
    //message boundaries
    struct sockaddr_in source;    
    circular_buffer_read(&usk->isk.sk.recv, &source, sizeof(struct sockaddr_in), 1);

    //set the msg name and len
    if(msg->msg_name && msg->msg_namelen >= sizeof(struct sockaddr_in)){
        
        memcpy(msg->msg_name, &source, sizeof(struct sockaddr_in));
        msg->msg_namelen = sizeof(struct sockaddr_in);
    }

    uint16_t recv_len;
    circular_buffer_read(&usk->isk.sk.recv, &recv_len, 2, 1);

    uint8_t* dst = msg->msg_iov->iov_base;
    size_t req_len = msg->msg_iov->iov_len;

    if(recv_len > req_len){
        msg->msg_flags = 1; //truncated
        circular_buffer_read(&usk->isk.sk.recv, dst, req_len, 1);
        
        //discard the rest
        for(size_t i = 0; i < (recv_len - req_len) ; ++i) 
            circular_buffer_getc(&usk->isk.sk.recv);  

        return req_len;

    }
    else{ //smaller so rest or the data is discarded

        circular_buffer_read(&usk->isk.sk.recv, dst, recv_len, 1);
        return recv_len;
    }
    

    return 0;
}


short udp_proto_ops_poll(file_t* file, struct poll_table* pt){
    
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct udp_sock* usk = socket->protocol_data;

    int min_size = sizeof(struct udp_recv); //least 2 bytes?
    int remaining = circular_buffer_avaliable(&usk->isk.sk.recv);
    //i do some ape shit here so i will just

    short mask = POLLOUT;
    if(remaining){
        mask |= POLLIN;
    }
   
    return mask;
}


//proto
int udp_proto_disconnect(struct sock *sk, int flags){
    //disconnect from what?
    struct inet_sock* isk  = (void*)sk;
    struct socket* socket = sk->sk_socket;

    if(socket->state == SS_LISTENING){

        
        int wordi = isk->bound_port / 32;
        int biti = isk->bound_port % 32;
        
        udp_port_bitmap[wordi] &= ~(1ul << biti);

        listnode_t* lnode = list_remove(&bound_sockets, (void*)socket->iconn);
        if(lnode){

            lnode->val = NULL;
            kfree(lnode);
        }
    }
    return 0;
}

void udp_proto_destroy(struct sock* sk){
    
    struct socket* socket = sk->sk_socket;   
    struct udp_sock* usk = (void*)sk;

    
    sk->sk_refcount--;
    if(!sk->sk_refcount){ //no references left so...
        memset(socket, 0, sizeof(struct socket));

        listnode_t* node;
        while( (node = list_pop_end(&sk->pending_writes)) ){
            kfree(node->val);
            kfree(node);
        }
    
        while( (node = list_pop_end(&sk->pending_reads)) ){
            kfree(node->val);
            kfree(node);
        }

        circular_buffer_destroy(&sk->recv);
        
        memset(usk, 0, sizeof(struct udp_sock)); 
        kfree(sk);
    }

}





struct proto_ops udp_proto_ops = {
    .bind = udp_proto_ops_bind,
    .accept = udp_proto_ops_accept,
    .connect = udp_proto_ops_connect,
    .sendmsg = udp_proto_ops_sendmsg,
    .recvmsg = udp_proto_ops_recvmsg,
    .sendto = udp_proto_ops_sendto,
    .poll = udp_proto_ops_poll

};

struct proto udp_prot = {
    .disconnect = udp_proto_disconnect,
    .destroy = udp_proto_destroy,
};



int udp_create_sock(struct socket* socket){

    struct udp_sock *usk = kcalloc(1, sizeof(struct udp_sock));
    if(!usk)
        return -ENOBUFS;
    
    struct sock* sk = &usk->isk.sk;
    sk->sk_refcount = 1;
    sk->sk_socket = socket;
    sk->sk_prot = &udp_prot;
    sk->sk_proc = current_process->pid;

    sk->recv = circular_buffer_create(4096);
    sk->pending_reads = list_create();
    sk->pending_writes = list_create();

    usk->isk.bound_ip = 0;
    usk->isk.bound_port = 0;

    socket->protocol_data = sk;
    socket->ops = &udp_proto_ops;

    return 0;
}            






int udp_scoket_bound(struct socket* socket, struct sockaddr* addr, socklen_t len){
    
    struct sockaddr_in* saddr = (struct sockaddr_in*)addr;
    int port = ntohs(saddr->sin_port);
    //check if that port is allocated or not
    size_t word_index = port / 32;
    size_t bit_index = port % 32;

    //already bound
    if(GET_BIT(udp_port_bitmap[word_index], bit_index)){
        return -1;
    }

    udp_port_bitmap[word_index] |= 1ul << bit_index;

    struct sock* sock = socket->protocol_data;
    
    return 0;
}


uint16_t udp_calculate_checksum(struct udp* udp, uint8_t* destip, uint8_t* senderip){

    /*
    +-------------------------+
    | Source address - 4 Byte |
    +-------------------------+
    |  Dest   Adress - 4 Byte |
    +-------------------------+---------------------+
    | zero-byte | protocol-Byte| udp-length - 2 Byte|
    +-----------------------------------------------+
    |           UDP                                 |
    +-----------------------------------------------+
    */

    size_t udp_length = ntohs(udp->length);
    size_t total_length = ntohs(udp->length) + 12;

    struct pseudo_header {
        uint8_t src_ip[4];
        uint8_t dst_ip[4];
        uint8_t zero;
        uint8_t protocol;
        uint16_t udp_length;
    };

    struct pseudo_header *ph = kcalloc(1, total_length);
    memcpy(ph->src_ip, senderip, 4);
    memcpy(ph->dst_ip, destip, 4);
    ph->zero = 0;
    ph->protocol = IPV4_PROTOCOL_UDP; // Usually 17
    ph->udp_length = udp->length; // already in network byte order


    struct udp* psudp = (struct udp*)&ph[1];

    memcpy(psudp, udp, udp_length);    
    psudp->checksum = 0;

    uint16_t checksum = compute_checksum((uint16_t*)ph, total_length);
    if(checksum == 0) checksum = 0xFFFF;
    udp->checksum = checksum;
    kfree(ph);
    return checksum;
}

int udp_net_send_socket(const struct ipv4_packet* ip, size_t len){

    struct udp *udp = (struct udp *)((uint8_t*)ip + (ip->version_ihl & 0xF)*0x4);
    uint8_t* recv_data = udp->data;
    size_t recv_data_length = ntohs(udp->length) - sizeof(struct udp);
    // kxxd(recv_data, recv_data_length);

    // fb_console_printf("received an udp packet\n");
    
    for(listnode_t* node = bound_sockets.head; node; node = node->next){
        struct socket* socket = node->val;
        struct udp_sock* usk = socket->protocol_data;

        if(ntohs(udp->dport) == usk->isk.bound_port){ //check port


            if(!usk->isk.bound_ip || !memcmp(&usk->isk.bound_ip, ip->tpa, 4 ) ){ //IN_ANY //any adress or the intended ip addr
                /*
                
                struct sockaddr_in source;
                uint16_t payload_len;
                uint8_t payload[]
                */

                pcb_t* proc = process_get_by_pid(usk->isk.sk.sk_proc);
                if(proc->state == TASK_INTERRUPTIBLE)
                    proc->state = TASK_RUNNING;
                
                
                struct sockaddr_in source;
                source.sin_family = AF_INET;
                source.sin_port = udp->sport;
                memcpy(&source.sin_addr, ip->spa, 4);
                    
                circular_buffer_write(&usk->isk.sk.recv, &source, sizeof(struct sockaddr_in), 1);
                circular_buffer_write(&usk->isk.sk.recv, (uint16_t*)&recv_data_length, sizeof(uint16_t), 1);
                circular_buffer_write(&usk->isk.sk.recv, recv_data, recv_data_length, 1);
                return recv_data_length;

            }
        }
        
    }
    return 0;
}