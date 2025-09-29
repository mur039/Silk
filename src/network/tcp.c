#include <network/inet_socket.h>
#include <network/tcp.h>
#include <syscalls.h>


int tcp_output(const struct sk_buff* skb_in, uint8_t ctrl_flag, uint32_t seq, uint32_t ack, void* data, size_t data_len);
#define TCP_HASH_MAX_BUCKETS 65536
list_t tcp_bounds[TCP_HASH_MAX_BUCKETS] = {{.head = NULL, .tail = NULL, .size = 0}}; //okay lets go


struct tcp_sock* tcp_listen_lookup(uint32_t laddr, uint16_t lport){
    list_t* e = &tcp_bounds[lport];
    
    //exact match
    for(listnode_t* n = e->head; n ; n = n->next){
        struct tcp_sock* tsk = (struct tcp_sock*)n->val;
        if(tsk->isk.bound_ip == laddr && tsk->isk.sk.state == SS_LISTENING){
            return tsk;
        }
    }

    //inanyaddr
    for(listnode_t* n = e->head; n ; n = n->next){
        struct tcp_sock* tsk = (struct tcp_sock*)n->val;
        if(tsk->isk.bound_ip == 0 && tsk->isk.sk.state == SS_LISTENING){
            return tsk;
        }
    }

    return NULL;
}


struct tcp_sock* tcp_established_lookup(uint32_t laddr, uint16_t lport, uint32_t raddr, uint16_t rport){
    list_t* e = &tcp_bounds[lport];
    
    //exact match
    for(listnode_t* n = e->head; n ; n = n->next){
        struct tcp_sock* tsk = (struct tcp_sock*)n->val;
        if(tsk->isk.bound_ip == laddr && tsk->isk.connected_ip == raddr && tsk->isk.connected_port == rport && tsk->isk.sk.state == SS_CONNECTED){
            return tsk;
        }
    }
    return NULL;
}



struct tcp_sock* tcp_connecting_lookup(uint32_t laddr, uint16_t lport, uint32_t raddr, uint16_t rport){
    list_t* e = &tcp_bounds[lport];
    
    //exact match
    for(listnode_t* n = e->head; n ; n = n->next){
        struct tcp_sock* tsk = (struct tcp_sock*)n->val;
        if((tsk->isk.bound_ip == laddr) && (tsk->isk.connected_ip == raddr) && (tsk->isk.connected_port == rport) && ((tsk->tcp_state == TCP_SYNSENT) || tsk->tcp_state == TCP_SYNRECEIVED)){
            return tsk;
        }
    }

    return NULL;
}


//proto_ops
int tcp_proto_ops_bind(file_t* file, struct sockaddr* addr, socklen_t len){

    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct tcp_sock *tsk = socket->protocol_data;
    struct inet_sock* isk = &tsk->isk;
    struct sock* sk = &isk->sk;

    struct sockaddr_in* iaddr = (struct sockaddr_in*)addr;
    
    if(sk->state != SS_UNCONNECTED){
        return -EINVAL;
    }

    uint32_t ip_addr = ntohl(iaddr->sin_addr);
    uint32_t port = ntohs(iaddr->sin_port);
    

    if(tcp_listen_lookup(ip_addr, port)){
        return -EADDRINUSE;        
    }

    isk->sk.state = SS_LISTENING;
    isk->bound_ip = ip_addr;
    isk->bound_port = port;

    //then push to the list
    list_insert_end( &tcp_bounds[port], tsk);
    return 0;
}



int tcp_proto_ops_accept(file_t* file, struct sockaddr* addr, socklen_t* len){
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct tcp_sock *tsk = socket->protocol_data;
    struct inet_sock* isk = &tsk->isk;
    struct sock* sk = &isk->sk;
    

    interruptible_sleep_on(&sk->accept_waitqueue, socket->iconn);

    struct socket* nsocket = socket->iconn;
    socket->iconn = nsocket->iconn;

    int fd = process_get_empty_fd(current_process);
    if(fd < 0){
        //actually delete the socket too
        return -EMFILE;
    }    

    file_t* nfile = &current_process->open_descriptors[fd];
    nfile->f_inode = kcalloc(1, sizeof(struct fs_node));
    nfile->f_mode = 0664;
    nfile->f_flags = O_RDWR;
    nfile->f_pos = 0;

    struct fs_node* nfnode = (struct fs_node*)nfile->f_inode;
    nfnode->flags = FS_SOCKET;
    nfnode->device = nsocket;
    nfnode->ctime = get_ticks();
    nfnode->atime = nfnode->ctime;
    nfnode->mtime = nfnode->ctime;
    nsocket->file = nfile;

    //set the fields
    if(addr && len){
        struct sockaddr_in* iaddr = (struct sockaddr_in*)addr;
        iaddr->sin_family = AF_INET;
        iaddr->sin_addr = ((struct tcp_sock*)nsocket->protocol_data)->isk.connected_ip;
        iaddr->sin_port = htons(((struct tcp_sock*)nsocket->protocol_data)->isk.connected_port);
    }

    return fd;
}

int tcp_proto_ops_recvmsg(file_t *file, struct msghdr* msg, int flags){
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct tcp_sock* tsk = socket->protocol_data;
    struct sock* sk = &tsk->isk.sk;
    struct iovec* vec = msg->msg_iov;
    uint8_t* userbuff = vec->iov_base;
    
    for(size_t i = 0;  i < vec->iov_len; ){
        

        struct sk_buff* pack;
        while(!(pack = sk->rx_head)) {
            //check whether socket is readable
            if(tsk->tcp_state != TCP_ESTABLISHED){
                return i;
            }
            sleep_on(&sk->rx_waitqueue); // recheck after wakeup
        }   


        if(pack->len < vec->iov_len){ //pop and eat
            
            if(sk->rx_head == sk->rx_tail){
                sk->rx_head = NULL;
                sk->rx_tail = NULL;
            }
            else{
                sk->rx_head = pack->next;
            }

            memcpy(userbuff + i, pack->data, pack->len);
            i += pack->len;
            skb_free(pack);
        }
        else{ //don pop but eat
            memcpy(&userbuff[i], pack->data, 1);
            skb_pull(pack, 1);
            i++;
        }


    }

    return vec->iov_len;
}



int tcp_proto_ops_sendmsg(file_t* file, void* addr, size_t len){
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct tcp_sock* tsk = socket->protocol_data;
    struct sock* sk = &tsk->isk.sk;

    if(tsk->tcp_state != TCP_ESTABLISHED){
        return -EINVAL;
    }

    //my idiomacy make it come from outside
    struct sk_buff* skb = skb_alloc(20 + 64 + len);
    skb_reserve(skb, 64);
    skb_put(skb, len);
    struct tcp *tcp = skb_push(skb, 20);
    
    tcp->destination_port = htons(tsk->isk.bound_port);     // should be connected_port
    tcp->source_port      = htons(tsk->isk.connected_port); // should be bound_port
    skb->src_ip           = (tsk->isk.connected_ip);        // should be bound_ip
    skb->dst_ip           = (tsk->isk.bound_ip);            // should be connected_ip

    skb->dev = NULL;
    
    tcp_output(skb, (PSH | ACK), tsk->snd_next, tsk->rcv_next, addr, len);
    tsk->snd_next += len;

    skb_free(skb);
    return len;
}




int tcp_proto_ops_connect(file_t* file, struct sockaddr* addr, socklen_t len){
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
    return 0;
}

void tcp_proto_destroy(struct sock *sk){
    struct tcp_sock* tsk = (struct tcp_sock*)sk;
    struct inet_sock* isk = (struct  inet_sock*)sk;


    list_t *e = &tcp_bounds[isk->bound_port];
    for(listnode_t* n = e->head; n ; n = n->next){
        struct tcp_sock* etsk = (struct tcp_sock*)n->val;
        if(etsk == tsk){
            list_remove(e, n);
            kfree(n);
            break;
        }
    }

    for(struct sk_buff* head = sk->rx_head; head; head = head->next){
        skb_free(head);
    }

    kfree(tsk);
    return;
}

struct proto tcp_proto = {
    .disconnect = tcp_proto_disconnect,
    .destroy = tcp_proto_destroy,
};



int tcp_create_socket(struct socket* socket){

    struct tcp_sock *tsk = kcalloc(1, sizeof(struct tcp_sock));
    if(!tsk){
        return -ENOMEM;
    }

    tsk->tcp_state = TCP_CLOSED;
    struct sock* sk = &tsk->isk.sk;
    sk->sk_refcount++;
    sk->sk_socket = socket;
    sk->sk_proc = current_process->pid;
    sk->state = SS_UNCONNECTED;

    sk->pending_reads = list_create();
    sk->pending_writes = list_create();

    socket->protocol_data = sk;
    
    sk->sk_prot = &tcp_proto;
    socket->ops = &tcp_proto_ops;
    
    return 0;
}

struct tcp_sock* tcp_create_child(struct tcp_sock* parent, const struct sk_buff* skb){
    
    struct tcp* tcp = (struct tcp*)skb->data;
    struct socket * socket = socket_allocate_socket();
    if(!socket){
        return NULL;
    }
    tcp_create_socket(socket);

    struct sock* sk = (struct sock*)socket->protocol_data;
    struct tcp_sock* nsk = (struct tcp_sock*)sk;

    nsk->isk.bound_ip = skb->dst_ip;
    nsk->isk.bound_port = ntohs(tcp->destination_port);

    nsk->isk.connected_ip = skb->src_ip;
    nsk->isk.connected_port = ntohs(tcp->source_port);
    nsk->tcp_state = TCP_CLOSED;
    nsk->rcv_next = ntohl(tcp->seq_numb) + 1;
    uint32_t Y = 0; //initial sequence number
    nsk->snd_next = Y;
    nsk->snd_una = Y;

    list_t* e = &tcp_bounds[nsk->isk.bound_port];
    list_insert_end(e, nsk);
    return nsk;
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

int tcp_send_ctrl_seq(const struct sk_buff* skb_in, uint8_t ctrl_flag, uint32_t seq_numb){
    
    const struct tcp* tcp = (const struct tcp*)skb_in->data;
    struct sk_buff* rst = skb_alloc(20 + 64); //create whole thing
    rst->protocol = IPV4_PROTOCOL_TCP;
    rst->dst_ip = skb_in->src_ip;
    rst->src_ip = skb_in->dst_ip;
    rst->dev = skb_in->dev;

    skb_reserve(rst, 64); //reserve some for ip and ethernet
    struct tcp* rtcp = skb_push(rst, 20); //nopayload, no options just tcpheader
    
    //set teh fields
    rtcp->urgent_ptr = 0;
    rtcp->destination_port = tcp->source_port;
    rtcp->source_port = tcp->destination_port;
    rtcp->seq_numb = htonl(seq_numb);
    if(ctrl_flag == ACK){
        size_t data_off = (tcp->data_off_rsrvd_ctrl >> 12) & 0x3f;
        size_t datalen = skb_in->len - data_off;
        rtcp->ack_numb = htonl(ntohl(tcp->seq_numb) + datalen);
    }
    else{

        rtcp->ack_numb = htonl(ntohl(tcp->seq_numb) + 1);
    }
    rtcp->data_off_rsrvd_ctrl = htons( (0x5 << 12) | (ctrl_flag & 0x3f) );
    rtcp->window = tcp->window;
    rtcp->checksum = 0;
    rtcp->checksum  = tcp_package_calc_checksum(rtcp, 20, rst->src_ip, rst->dst_ip);

    ip_output(rst);
    return 0;
}

int tcp_output(const struct sk_buff* skb_in, uint8_t ctrl_flag, uint32_t seq, uint32_t ack, void* data, size_t data_len){
    
    const struct tcp* tcp = (const struct tcp*)skb_in->data;
    
    struct sk_buff* rst = skb_alloc(data_len + 20 + 64); //data + tcp header+ future ip and eth header
    rst->protocol = IPV4_PROTOCOL_TCP;
    rst->dst_ip = skb_in->src_ip;
    rst->src_ip = skb_in->dst_ip;
    rst->dev = skb_in->dev;

    skb_reserve(rst, 64); //reserve some for ip and ethernet
    void* payload = skb_put(rst, data_len);
    memcpy(payload, data, data_len);

    struct tcp* rtcp = skb_push(rst, 20); // tcpheader no options
    
    //set teh fields
    rtcp->urgent_ptr = 0;
    rtcp->destination_port = tcp->source_port;
    rtcp->source_port = tcp->destination_port;
    rtcp->seq_numb = htonl(seq);
    rtcp->ack_numb = htonl(ack);
    
    rtcp->data_off_rsrvd_ctrl = htons( (0x5 << 12) | (ctrl_flag & 0x3f) );
    rtcp->window = htons(4096);
    rtcp->checksum = 0;
    rtcp->checksum  = tcp_package_calc_checksum(rtcp, 20 + data_len, rst->src_ip, rst->dst_ip);

    ip_output(rst);
    return 0;
}


int tcp_input(struct sk_buff* skb){
    struct ipv4_packet* ip = (struct ipv4_packet*)skb->data;
    size_t iphdr_len = (ip->version_ihl & 0xf) * 4;
    struct tcp* tcp = (struct tcp*)skb_pull(skb, iphdr_len);
    //okay some tcp schenaningans
    
    //get some flags
    uint16_t drc = ntohs(tcp->data_off_rsrvd_ctrl);
    uint16_t data_start_offset =  ((drc  >> 12) & 0xF) * 4;
    uint8_t ctrl = drc &  0x3F; 

    size_t ip_total_len = ntohs(ip->total_length);
    size_t tcp_total_len = ip_total_len - iphdr_len ;
    size_t tcp_hdrlen = data_start_offset ;
    size_t tcp_payload_len = tcp_total_len - tcp_hdrlen;


    //but first let's find a socket
    struct tcp_sock* socket = tcp_established_lookup(skb->dst_ip, ntohs(tcp->destination_port), skb->src_ip, ntohs(tcp->source_port));
    if(socket){

        uint32_t ack = ntohl(tcp->ack_numb);
        struct sock* sk = &socket->isk.sk;
        if(ctrl == (PSH | ACK)){
            
            //slide if necessary
            if( (ack > socket->snd_una) && ( ack <= socket->snd_next) ){
                socket->snd_una = ack;
            }
            //ack back to the sender
            socket->rcv_next = ntohl(tcp->seq_numb) +  tcp_payload_len;
            tcp_output(skb, ACK, socket->snd_next, socket->rcv_next, NULL, 0);
            
            skb_pull(skb, data_start_offset); //just payload
            skb_trim(skb, tcp_payload_len); //just to trim it at the end?
            
            //queue it up
            if(!sk->rx_head){ //no message
                sk->rx_head = skb;
                sk->rx_tail = skb;
                
            }
            else{
                sk->rx_tail->next = skb;
                sk->rx_tail = skb;
            }
            process_wakeup_list(&sk->rx_waitqueue);
            return 1;
        }

        if(ctrl & FIN){
            socket->rcv_next = ntohl(tcp->seq_numb) + 1;    
            tcp_output(skb, ACK, socket->snd_next, socket->rcv_next, NULL, 0);
            socket->tcp_state = TCP_CLOSEWAIT;
            socket->isk.sk.state = SS_DISCONNECTING;
            socket->isk.sk.sk_socket->state = SS_DISCONNECTING;
            
            skb_free(skb);
            process_wakeup_list(&sk->rx_waitqueue);
            return 1;
        }   

        if(ctrl == ACK){ //keep-alive or ack to our psh 
            
            if( (ack > socket->snd_una) && ( ack <= socket->snd_next) ){
                socket->snd_una = ack;
            }

            tcp_output(skb, ACK, socket->snd_next, socket->rcv_next, NULL, 0);
            return 1;
        }

    }


    if((ctrl & SYN) ){ //handshake

        if(ctrl == SYN ){ //first handshake
            socket = tcp_listen_lookup(skb->dst_ip, ntohs(tcp->destination_port));
            if(socket){
                //as well as somethin
                struct tcp_sock* child = tcp_create_child(socket, skb);
                if(child){

                    tcp_output(skb, SYN | ACK, child->snd_next, child->rcv_next, NULL, 0);
                    child->tcp_state = TCP_SYNRECEIVED;
                    child->snd_next += 1;
                    skb_free(skb);
                    return 0;
                }
            }
        }
    }
    
    if(ctrl == ACK){ //third handshake
    
        struct tcp_sock* socket = tcp_connecting_lookup(skb->dst_ip, ntohs(tcp->destination_port), skb->src_ip, ntohs(tcp->source_port));
        if(socket){
                
            if(ntohl(tcp->ack_numb) == socket->snd_next){ //we send seq=0 
                socket->tcp_state = TCP_ESTABLISHED;
                socket->isk.sk.state = SS_CONNECTED;
                socket->isk.sk.sk_socket->state = SS_CONNECTED;
                
                size_t datalen = skb->len - data_start_offset; //even though its zero
                socket->rcv_next = ntohl(tcp->seq_numb) + datalen; // actually also + tcp_data_len 
                socket->snd_una = ntohl(tcp->ack_numb);
                //wakeup the parent socket?
                struct tcp_sock* parent = tcp_listen_lookup(skb->dst_ip, ntohs(tcp->destination_port));
                process_wakeup_list(&parent->isk.sk.accept_waitqueue);
                
                //attach child socket to the parents iconn;
                socket->isk.sk.sk_socket->iconn = parent->isk.sk.sk_socket->iconn;
                parent->isk.sk.sk_socket->iconn = socket->isk.sk.sk_socket;
                skb_free(skb);
                return 0;
            }
        }
    }
        


    tcp_send_ctrl_seq(skb, (RST | ACK), 0);
    skb_free(skb); //release it
    return 1;
}
