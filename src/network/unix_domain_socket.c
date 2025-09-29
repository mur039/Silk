#include "socket.h"
#include "network/unix_domain_socket.h"
#include "syscalls.h"
#include <network/skb.h>
#include <syscalls.h>

list_t unix_bound_socks;

uint32_t uds_fsnode_read(fs_node_t* fnode, uint32_t offset, uint32_t count, uint8_t* buffer ){

    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data; 
    struct sock* sk = &sock->sk;



    for(size_t i = 0; i < count;){

        
        struct sk_buff* pack;
        while(!(pack = sk->rx_head)) {
            if(!sock->peer){ //karşı taraf kapanmışsa 
                return 0; //eof
            }
            sleep_on(&sk->rx_waitqueue); // recheck after wakeup
        }   

        //message boundary
        if(socket->type == SOCK_DGRAM){

            //if i have package i pop it off
            if(sk->rx_head == sk->rx_tail){
                sk->rx_head = NULL;
                sk->rx_tail = NULL;
            }
            else{
                sk->rx_head = pack->next;
            }
            memcpy(buffer, pack->data, pack->len);
            skb_free(pack);
            return pack->len;
        }
        else if(socket->type == SOCK_STREAM){

            if(pack->len <= (count - i)){
                if(sk->rx_head == sk->rx_tail){
                    sk->rx_head = NULL;
                    sk->rx_tail = NULL;
                }
                else{
                    sk->rx_head = pack->next;
                }
                memcpy(buffer + i, pack->data, pack->len);
                i += pack->len;
                skb_free(pack);
            }
            else{
                memcpy(buffer + i, pack->data, 1);
                skb_pull(pack, 1);
                i++;
            }
            

        }
    }
    
    return count;
}


uint32_t uds_fsnode_write(fs_node_t* fnode, uint32_t offset, uint32_t count, uint8_t* buffer ){
    
    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data;
    struct unix_sock* peer = sock->peer;
    if(!peer){ //karşı taraf kapanmışsa 
        return -EIO; //eof //need EPIPE
    }
    
    //should check for space farling
    struct sk_buff* skb = skb_alloc(count);
    memcpy(skb_put(skb, count), buffer, count);
    
    struct sock *sk = &peer->sk;

    if(sk->rx_tail){
        sk->rx_tail->next = skb;
        sk->rx_tail = skb;
    }
    else{
        sk->rx_head = skb;
        sk->rx_tail = skb;
    }

    process_wakeup_list(&sk->rx_waitqueue);
    return count;
}



    


int uds_bind(file_t *file, struct sockaddr * addr, socklen_t len){
    
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data; 
    struct sock* sk = &sock->sk;

    if(sk->state != SS_UNCONNECTED){
        return -EINVAL;
    }

    //first and foremost, check if there's another bound socket with same name
    struct sockaddr_un* un = (struct sockaddr_un*)addr;
    foreach((&unix_bound_socks), iter){
        struct unix_sock* s = iter->val;
        if(!strncmp(s->addr.sun_path, un->sun_path, 108)){
            return -EADDRINUSE;
        }
    }
    
    //copy the data to the protocol data
    memcpy(&sock->addr, addr, sizeof(sock->addr) );
    sk->state = SS_LISTENING;
    sk->max_ack_backlog = 0;

    //push it to the bound list
    list_insert_end(&unix_bound_socks, sock);
    return 0;
}

int uds_connect(file_t *file, const struct sockaddr* addr, socklen_t len){
    
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data;
    struct sock* sk = &sock->sk;

    if(sk->state != SS_UNCONNECTED){
        return -EINVAL;
    }

    struct sockaddr_un* un = (struct sockaddr_un*)addr;
    foreach((&unix_bound_socks), iter){
        struct unix_sock* s = iter->val;
        if(!strncmp(s->addr.sun_path, un->sun_path, 108)){
            
            if(s->sk.ack_backlog >= s->sk.max_ack_backlog){ //no more backlogging
                return -26; /// need  connrefuse...
            }

            //append it to the socket list
            s->sk.ack_backlog++;
            socket->iconn = s->sk.sk_socket->iconn;
            s->sk.sk_socket->iconn = socket; 
            
            sk->state = SS_CONNECTING;
            process_wakeup_list(&s->sk.accept_waitqueue);
            //okay we are going to bastardize the accept queue, 
            //since connecting sockets have no use we gonna write some dogshit code
            interruptible_sleep_on(&sk->accept_waitqueue, sk->state == SS_CONNECTED);
            return 0;
        }
    }

    //no such socket
    return -EINVAL;
}


int uds_accept(file_t *file, struct sockaddr *addr, socklen_t *len){
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data; 
    struct sock* sk = &sock->sk;
    
    interruptible_sleep_on(&sk->accept_waitqueue, socket->iconn);

    //pop connection socket
    struct socket* peer_socket = socket->iconn;
    socket->iconn = socket->next;

    struct unix_sock* peer_sock = peer_socket->protocol_data;

    //create new file entry for the accepting process
    int fd = process_get_empty_fd(current_process);
    if(fd < 0)
        return -EMFILE;
    
    file_t* sfile = &current_process->open_descriptors[fd];
    sfile->f_inode = kcalloc(1, sizeof(struct fs_node));
    sfile->f_flags = O_RDWR;
    sfile->f_mode = O_RDWR;
    sfile->f_pos = 0;

    //create a new child socket
    struct socket* child_socket = unix_create_socket(peer_socket->type, 0);
    struct unix_sock* child_uds = child_socket->protocol_data;

    peer_sock->peer = child_uds;
    child_uds->peer = peer_sock;
    
    peer_sock->sk.state = SS_CONNECTED;
    child_uds->sk.state = SS_CONNECTED;

    //somehow wakeup the children?
    //bastardize the accep queue of the connecting socket?
    process_wakeup_list(&peer_sock->sk.accept_waitqueue);

    fs_node_t* child_node = (fs_node_t*)sfile->f_inode;
    child_node->flags = FS_SOCKET;
    child_node->device = child_socket;

    child_node->ctime = get_ticks();
    child_node->atime = child_node->ctime;
    child_node->mtime = child_node->ctime;

    child_socket->file = sfile;
    return fd;
}



int uds_sendmsg(file_t *file, void *addr, size_t  len){
    return uds_fsnode_write((fs_node_t*)file->f_inode, 0, len, (uint8_t*)addr);
    
}

int uds_recvmsg(file_t *file, struct msghdr* msg, int flags){

    return uds_fsnode_read((fs_node_t*)file->f_inode, 0, msg->msg_iov->iov_len, (uint8_t*)msg->msg_iov->iov_base);
    
}

int uds_proto_disconnect(struct sock* sk, int flags){

    struct unix_sock *sock = (struct unix_sock *)sk;
    if (sock->peer){  //connected then

        sock->peer->peer = NULL;
        if(sock->peer->sk.ack_backlog)
            sock->peer->sk.ack_backlog--;
    }

    if (sock->sk.sk_socket->conn){ //probably a listener

        sock->peer->sk.sk_socket->conn = NULL;
        sock->peer->sk.sk_socket->state = SS_UNCONNECTED;
    }

    
    sock->peer = NULL;
    sock->sk.sk_socket->conn = NULL;
    sock->sk.sk_socket->state = SS_DISCONNECTING;

    return 0;
}

void uds_proto_destroy(struct sock* sk){
    struct socket* socket = sk->sk_socket;
    struct unix_sock* sock = (struct unix_sock*)sk;

    for(struct sk_buff* skb = sock->sk.rx_head; skb; skb = skb->next){
        skb_free(skb);
    }

    sock->sk.sk_socket->state = SS_FREE;
    
    if(sk->max_ack_backlog){ //binded socket?

        foreach((&unix_bound_socks), iter){
            struct unix_sock* s = iter->val;
            if(s == sock){
                list_remove(&unix_bound_socks, iter);
                kfree(iter);
                break;
            }
        }
    }
    kfree(sk);
}



short unix_ops_poll(file_t* file, struct poll_table* pt){
    
    struct fs_node* fnode = (struct fs_node*)file->f_inode;
    struct socket* socket = fnode->device;
    struct unix_sock* usk = socket->protocol_data;
    struct sock* sk = &usk->sk;

    
    //i do some ape shit here so i will just
    
    short mask = 0;

    if(sk->state == SS_CONNECTED){
        poll_wait(fnode, &sk->rx_waitqueue, pt);
        struct sk_buff* head = sk->rx_head;
        if(head != NULL){
            mask = POLLIN | POLLOUT;
        }
    }
    else if(sk->state == SS_LISTENING){
        if(socket->iconn){
            mask |= POLLIN;
        }
        else{
            poll_wait(fnode, &sk->accept_waitqueue, pt);
        }
    }
    
    return mask;
}


struct proto_ops uds_stream_ops = {
                                    .bind = uds_bind,
                                    .connect = uds_connect,
                                    .accept = uds_accept,
                                    .sendmsg = uds_sendmsg,
                                    .recvmsg = uds_recvmsg,
                                    .poll = unix_ops_poll
                                    
                                };


struct proto uds_stream_proto = {
                                .disconnect = uds_proto_disconnect,
                                .destroy    = uds_proto_destroy
                                
                                
};

struct socket* unix_create_socket(int type, int options){

    struct socket* socket = socket_allocate_socket();
    memset(socket, 0, sizeof(struct socket));

    if(!socket) //out of sockets
        return NULL;

    socket->type = type;
    socket->state = SS_UNCONNECTED;
    socket->flags = 0;
    socket->iconn = NULL;

    socket->ops = &uds_stream_ops;

    struct unix_sock* sk = kcalloc(sizeof(struct unix_sock), 1);
    socket->protocol_data = (void*)sk;

    sk->sk.state = SS_UNCONNECTED;
    sk->sk.sk_refcount = 1;
    sk->sk.sk_socket = socket;
    sk->sk.sk_proc = current_process->pid;
    sk->sk.sk_prot = &uds_stream_proto;

    return socket;

}