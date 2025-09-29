#include "network/udp.h"
#include "network/netif.h"
#include "network/ipv4.h"
#include "network/arp.h"
#include "network/route.h"
#include <network/skb.h>
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
    
    uint32_t ipv4_addr = (iaddr->sin_addr);
    uint16_t port = ntohs(iaddr->sin_port);
    uint8_t* ipaddress = (uint8_t*)&ipv4_addr;

    

    size_t total_len = len + sizeof(struct udp);
    struct sk_buff* skb = skb_alloc(total_len + 64);
    if(!skb){
        return -ENOMEM;
    }


    //check if socket is bound or not
    skb->protocol = IPV4_PROTOCOL_UDP;
    skb->dst_ip = ipv4_addr;
    skb->dev = socket->bound_if;

    //ugly as shi
    if(usk->isk.bound_ip == 0){

        if(!skb->dev){
            const route_t* r = route_select_route(skb->dst_ip);
            if(!r){
                skb_free(skb);
                return -ENOENT;
            }
            skb->dev = r->interface;
        }
        usk->isk.bound_ip = skb->dev->ip_addrs;   
    }


    if(usk->isk.bound_port == 0){

        struct sockaddr_in ead;
        int port = udp_get_ephemeral_port();
        if(port < 0){
            skb_free(skb);
            return -EADDRINUSE;
        }

        ead.sin_family = AF_INET;
        ead.sin_port = htons(port);
        ead.sin_addr = htonl(skb->dev->ip_addrs);
        int err = udp_proto_ops_bind(file, (struct sockaddr*)&ead, sizeof(ead) );
        if(err < 0) return err;
    }

    skb->src_ip = usk->isk.bound_ip;


    skb_reserve(skb, 64);
    memcpy(skb_put(skb, len), buf, len); //copy payload to its buffer
    struct udp *udp = skb_push(skb, sizeof(struct udp));
    udp->checksum = 0;
    udp->length = htons( sizeof(struct udp) + len );
    udp->sport = htons(usk->isk.bound_port);
    udp->dport = htons(port);

    uint32_t dstip = (*(uint32_t*)ipaddress);
    uint32_t senderip = (skb->src_ip) ;
    udp_calculate_checksum(udp, (uint8_t*)&dstip, (uint8_t*)&senderip );

    return ip_output(skb);
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


    struct sk_buff* package = usk->isk.sk.rx_head;
    if(!package){ //no message
        list_insert_end(&usk->isk.sk.rx_waitqueue, current_process);
        return -EAGAIN;
    }
    else{
        //consume the package
        if(usk->isk.sk.rx_head == usk->isk.sk.rx_tail){  //onepackage
            usk->isk.sk.rx_head = NULL;
            usk->isk.sk.rx_tail = NULL;
        }
        else{
            usk->isk.sk.rx_head = package->next;
        }

    }

    
    
    //message boundaries
    struct udp* udp = (struct udp*)package->data;
    if(msg->msg_name && msg->msg_namelen >= sizeof(struct sockaddr_in)){
        struct sockaddr_in source;    
        source.sin_family = AF_INET;
        source.sin_addr = package->src_ip;
        source.sin_port = udp->sport;
    
        //set the msg name and len
        memcpy(msg->msg_name, &source, sizeof(struct sockaddr_in));
        msg->msg_namelen = sizeof(struct sockaddr_in);
    }
    
    
    uint8_t* payload = (uint8_t*)skb_pull(package, sizeof(struct udp));
    //data reading?
    uint8_t* dst = msg->msg_iov->iov_base;
    size_t req_len = msg->msg_iov->iov_len;

    if(package->len > req_len){
        msg->msg_flags = 1; //truncated
    }
    else{ //smaller so rest or the data is discarded
        req_len = package->len;
    }
    memcpy(dst, payload, req_len);
    
    skb_free(package);
    return req_len;
}


short udp_proto_ops_poll(file_t* file, struct poll_table* pt){
    
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct udp_sock* usk = socket->protocol_data;

    
    struct sk_buff* head = usk->isk.sk.rx_head;
    //i do some ape shit here so i will just

    short mask = POLLOUT;
    if(head != NULL){
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


int udp_input(struct sk_buff* skb){
    struct ipv4_packet* ip = (struct ipv4_packet*)skb->data;
    struct udp* udp = (struct udp*)skb_pull(skb, sizeof(struct ipv4_packet));
    // uint8_t* payload = (uint8_t*)skb_pull(skb, sizeof(struct udp));
    
    //look in udp sockets
    for(listnode_t* node = bound_sockets.head; node; node = node->next){
        struct socket* socket = node->val;
        struct udp_sock* usk = socket->protocol_data;

        if(ntohs(udp->dport) == usk->isk.bound_port){ //check port

            if(!usk->isk.bound_ip || !memcmp(&usk->isk.bound_ip, ip->tpa, 4 ) ){ //IN_ANY //any adress or the intended ip addr
                
                //queue it to the socket
                if(!usk->isk.sk.rx_head){ //no message
                    usk->isk.sk.rx_head = skb;
                    usk->isk.sk.rx_tail = skb;
                    skb->next = NULL;
                }
                else{
                    usk->isk.sk.rx_tail->next = skb;
                    usk->isk.sk.rx_tail = skb;
                    skb->next = NULL;
                }

                process_wakeup_list(&usk->isk.sk.rx_waitqueue);
                return 1;
            }
        }
    }

    //not found then
    skb_free(skb);
    return 0;
}