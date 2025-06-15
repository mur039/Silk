#include "socket.h"
#include "network/unix_domain_socket.h"
#include "syscalls.h"




uint32_t uds_fsnode_read(fs_node_t* fnode, uint32_t offset, uint32_t count, uint8_t* buffer ){

    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data; 

    uint32_t remaining = circular_buffer_avaliable(&sock->sk.recv);
    if( remaining >= count){
        circular_buffer_read(&sock->sk.recv, buffer, 1, count);
        return count;
    }

    //aslında buffer'da kalanlarada bakmak lazım ama neyse
    if(!remaining && !sock->peer){ //recv size 0 ve karşı taraf kapanmışsa 
        return 0; //eof
    }

    //not enough so
    return -1;
}

uint32_t uds_fsnode_write(fs_node_t* fnode, uint32_t offset, uint32_t count, uint8_t* buffer ){
    
    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data; 


    //aslında buffer'da kalanlarada bakmak lazım ama neyse
    if(!socket->conn){ //karşı taraf kapanmışsa 
        return 0; //eof
    }


    //should check for space farling
    circular_buffer_write(&sock->peer->sk.recv, buffer, 1, count);

    //wake-up the client
    pcb_t* proc = process_get_by_pid(sock->peer->sk.sk_proc);
    if(proc && proc->state == TASK_INTERRUPTIBLE){
        proc->state = TASK_RUNNING;
    }
    
    return count;
}









static struct socket* uds_sockets = NULL;
static struct socket* uds_binded_sockets = NULL;


int uds_bind(file_t *file, struct sockaddr * addr, socklen_t len){
    
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data; 
    
    for(struct socket* server = uds_binded_sockets; server; server = server->next ){
        
        struct unix_sock* ssock = server->protocol_data;
        if(!memcmp(&ssock->addr, addr, len)){
            return -7; // addrinuse
        }
    }

    //if no same address is here then
    memcpy(&sock->addr, addr, sizeof(sock->addr) );
    
    //add this socket to binded socket list
    socket->next = uds_binded_sockets;
    uds_binded_sockets = socket;
    return 0; //success
}

int uds_connect(file_t *file, const struct sockaddr* addr, socklen_t len){
    
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    struct socket* socket = fnode->device;
    struct unix_sock* sock = socket->protocol_data; 

    for(struct socket* server = uds_binded_sockets; server; server = server->next ){
        
        struct unix_sock* ssock = server->protocol_data;
        if(server->state != SS_LISTENING) 
            continue;

        if(server->type != socket->type) 
            continue;

        if(sock->addr.sun_family != addr->sa_family) 
            continue;

        if(strncmp(ssock->addr.sun_path, addr->sa_data, 108))
            continue;

        //check if server has enough backlog
        if(ssock->sk.ack_backlog >= ssock->sk.max_ack_backlog){
            return -6; //connrefuse?
        }

        ssock->sk.ack_backlog++;
        socket->state =  SS_CONNECTING;
        // like waiting list
        socket->next = server->iconn;
        server->iconn = socket;
        //then  wakeup the server process
        pcb_t* sproc = process_get_by_pid(ssock->sk.sk_proc);
        if(sproc->state == TASK_INTERRUPTIBLE){
            sproc->state = TASK_RUNNING;
        }
        return 0;
    }   
    

    //no such socket
    return -EINVAL;
}

int uds_accept(file_t *file, struct sockaddr *addr, socklen_t *len){
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    struct socket* socket = fnode->device;
    
    if(!socket->iconn) 
        return -EAGAIN;

    struct socket* csocket = socket->iconn;
    struct unix_sock* csock = csocket->protocol_data;
    socket->iconn = socket->next;

    //create new file entry for the accepting process
    int fd = process_get_empty_fd(current_process);
    if(fd < 0)
        return -EMFILE;
    
    file_t* sfile = &current_process->open_descriptors[fd];
    sfile->f_flags = O_RDWR;
    sfile->f_mode = O_RDWR;
    sfile->f_pos = 0;

    
    struct socket* ssocket = unix_create_socket(csocket->type, 0);
    struct unix_sock* ssock = ssocket->protocol_data;

    ssock->peer = csock;
    csock->peer = ssock;
    
    ssocket->state = SS_CONNECTED;
    csocket->state = SS_CONNECTED;

    ssocket->conn = csocket;
    csocket->conn = ssocket;


    pcb_t* cproc = process_get_by_pid(csock->sk.sk_proc);
    if(cproc->syscall_state == SYSCALL_STATE_PENDING &&  cproc->state == TASK_INTERRUPTIBLE){
        if(cproc->syscall_number == SYSCALL_CONNECT){
            cproc->regs.eax = 0;
            cproc->syscall_state = SYSCALL_STATE_NONE;
            cproc->state = TASK_RUNNING;
        }
    }        

    //create buffers later

    fs_node_t* sfnode = kcalloc(1, sizeof(fs_node_t));
    sfnode->flags = FS_SOCKET;
    sfnode->device = ssocket;

    sfnode->ctime = get_ticks();
    sfnode->atime = sfnode->ctime;
    sfnode->mtime = sfnode->ctime;

    
    sfile->f_inode = (void*)sfnode;
    ssocket->file = sfile;


    

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
    if (sock->peer)
    {

        sock->peer->peer = NULL;
        if(sock->peer->sk.ack_backlog)
            sock->peer->sk.ack_backlog--;
    }

    if (sock->sk.sk_socket->conn)
    {

        sock->peer->sk.sk_socket->conn = NULL;
        sock->peer->sk.sk_socket->state = SS_UNCONNECTED;
    }

    

    sock->peer = NULL;
    sock->sk.sk_socket->conn = NULL;
    sock->sk.sk_socket->state = SS_UNCONNECTED;

    return 0;
}

void uds_proto_destroy(struct sock* sk){
    struct socket* socket = sk->sk_socket;
    struct unix_sock* sock = (struct unix_sock*)sk;
    
    circular_buffer_destroy(&sock->sk.recv);
    sock->sk.sk_socket->state = SS_FREE;
    
    if(sk->max_ack_backlog){ //binded socket?

        if(uds_binded_sockets == socket){
            uds_binded_sockets = socket->next;
        }
        else{

            for(struct socket* ssocket = uds_binded_sockets; ssocket; ssocket = ssocket->next){
                
                if(ssocket->next == socket){
                    ssocket->next = socket->next;
                }
            }
        }
    }
    kfree(sk);
}

struct proto_ops uds_stream_ops = {
                                    .bind = uds_bind,
                                    .connect = uds_connect,
                                    .accept = uds_accept,
                                    .sendmsg = uds_sendmsg,
                                    .recvmsg = uds_recvmsg
                                    
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

    socket->ops = &uds_stream_ops;

    struct unix_sock* sk = kcalloc(sizeof(struct unix_sock), 1);
    socket->protocol_data = (void*)sk;

    sk->sk.sk_refcount = 1;
    sk->sk.sk_socket = socket;
    sk->sk.sk_proc = current_process->pid;
    sk->sk.recv = circular_buffer_create(4096);
    sk->sk.sk_prot = &uds_stream_proto;


    //append it to the list as well
    socket->next = uds_sockets;
    uds_sockets = socket;
    
    return socket;

}