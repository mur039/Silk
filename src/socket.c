#include <fb.h>
#include <socket.h>
#include <syscalls.h>
#include <network/netif.h>
#include <network/unix_domain_socket.h>
#include <network/inet_socket.h>
#include <network/route.h>

size_t socket_count = 0;
struct socket* sockets = NULL;


uint32_t socket_fsread(fs_node_t* fnode, uint32_t offset, uint32_t length, uint8_t* buffer){

    struct iovec iov = {
        .iov_base = buffer,
        .iov_len = length
    };
    
    struct msghdr msg = {
        .msg_name = NULL,
        .msg_namelen = 0,
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0
    };
    
    struct socket* socket = fnode->device;
    int err = socket->ops->recvmsg(socket->file, &msg, 0);
    if(err == -EAGAIN || err == -1){
        return -1;
    }
    
}
uint32_t socket_fswrite(fs_node_t* fnode, uint32_t offset, uint32_t length, uint8_t* buffer){
    
    struct socket* socket = fnode->device;
    return socket->ops->sendmsg(socket->file, buffer, length);
}


/*
SIOCADDRT and SIOCDELRT take an input pointer whose type depends on the protocol:

Most protocols      const struct rtentry *
*/

int socket_inet_ioctl(fs_node_t* fnode, unsigned long opcode, void* argp){
    
    struct socket* socket = fnode->device;
    struct ifreq* req = NULL;
    struct nic* dev = NULL;

    if(!is_virtaddr_mapped(argp))  return -EFAULT;
    
    switch (opcode){
    case SIOCGIFCONF:
    ;
    struct ifconf* conf = argp;
    return netif_ioctl_list_interfaces(conf);
    break;
    
    case SIOCGIFADDR:
    ;
    req = argp;
    dev = netif_get_by_name(req->ifr_name);
    if(!dev)
        return -EINVAL;
    
    memcpy(&req->ifr_addr , &dev->ip_addrs, 4);
    
    break;

    case SIOCGIFHWADDR:
    ;
    req = argp;
    dev = netif_get_by_name(req->ifr_name);
    if(!dev)
        return -EINVAL;

    memcpy(req->ifr_hwaddr.sa_data , dev->mac, 6);
        
    break;

    case SIOCSIFADDR: //set ipaddr
        req = argp;
        dev = netif_get_by_name(req->ifr_name);
        if(!dev)
            return -EINVAL;

        //remove previous routes
        route_remove_routes_for_interface(dev);

        dev->ip_addrs = req->ifr_addr;
        dev->gateway = req->ifr_map.mem_end; // :/
        // dev->gateway &= 0xFFFFFF00;
        // dev->gateway |= 1;

        route_add_route(dev->ip_addrs & 0xFFFFFF00, 0xFFFFFF00, 0, dev); //private subnet
        route_add_route(0, 0, dev->gateway, dev); // default for routing
        
    break;

    default: 
        return -EINVAL; 
    break;
    }

    return 0;
}


void socket_close(fs_node_t* fnode){
    struct socket* socket = fnode->device;
    struct sock* sock = socket->protocol_data;
    if(sock->sk_prot){

        sock->sk_prot->disconnect(sock, 0);
        sock->sk_prot->destroy(sock);
    }
}

fs_node_t sock_node = {
    .flags = FS_SOCKET,
    .read = socket_fsread,
    .write = socket_fswrite,
    .close = socket_close
};

void initialize_sockets(int max_socket_count){

    install_syscall_handler(SYSCALL_SOCKET, syscall_socket );
    install_syscall_handler(SYSCALL_CONNECT, syscall_connect );
    install_syscall_handler(SYSCALL_LISTEN, syscall_listen );
    install_syscall_handler(SYSCALL_BIND, syscall_bind );
    install_syscall_handler(SYSCALL_ACCEPT, syscall_accept);
    install_syscall_handler(SYSCALL_SENDTO, syscall_sendto);
    install_syscall_handler(SYSCALL_SETSOCKOPT, syscall_setsockopt);
    install_syscall_handler(SYSCALL_RECVFROM, syscall_recvfrom);

    //allocate sockets
    socket_count = max_socket_count;
    sockets = kcalloc(max_socket_count, sizeof(struct socket));
    for(size_t i = 0; i < socket_count; ++i){
        sockets[i].state = SS_FREE;
    }
}


struct socket* socket_allocate_socket(){
    for(size_t i = 0; i < socket_count; ++i){
        if(sockets[i].state == SS_FREE){
            return &sockets[i];
        }
    }

    //if no free sockets then just
    return NULL;
}

//int socket(int domain, int type, int protocol){
void syscall_socket(struct regs* r){

    // fb_console_printf("syscall_socket\n");
    int domain = r->ebx;
    int type = r->ecx;
    int protocol = r->edx;

    //checks for domain, type ,protocol etc;


    int fd;
    file_t* file;
    fs_node_t* fnode;
    struct socket* socket;
    switch(domain){
        case AF_UNIX: 
            socket = unix_create_socket(type, protocol);
            if(!socket){
                r->eax = -ENOBUFS;
                return;
            }
            
            fd = process_get_empty_fd(current_process);
            if(fd < 0){
                r->eax = -1;
                break;
            }

            file =  &current_process->open_descriptors[fd];
            file->f_flags = O_RDWR;
            file->f_mode = O_RDWR;
            file->f_pos = 0;
            
            fnode = kcalloc(1, sizeof(fs_node_t));
            *fnode = sock_node;
            fnode->ioctl = NULL;
            fnode->device = socket;
            fnode->ctime = get_ticks();
            fnode->atime = fnode->ctime;
            fnode->mtime = fnode->ctime;

            file->f_inode = (void*)fnode;
            socket->file = file;
            
            r->eax = fd;
            break;
        
        case AF_INET:
            socket = inet_socket_create(type, protocol);
            if(!socket){
                r->eax = -ENOBUFS;
                break;

            }
             
            fd = process_get_empty_fd(current_process);
            if(fd < 0){
                r->eax = -1;
                break;
            }

            file =  &current_process->open_descriptors[fd];
            file->f_flags = O_RDWR;
            file->f_mode = O_RDWR;
            file->f_pos = 0;
            
            fnode = kcalloc(1, sizeof(fs_node_t));
            *fnode = sock_node;
            fnode->ioctl = socket_inet_ioctl;
            fnode->device = socket;
            fnode->ctime = get_ticks();
            fnode->atime = fnode->ctime;
            fnode->mtime = fnode->ctime;

            file->f_inode = (void*)fnode;
            socket->file = file;
            
            r->eax = fd;
        
        break;

        default : 
            fb_console_printf("That socket domain:%u, does not exist darling\n", domain);
            r->eax = -1;
            break;
    }

    
    return;
};


//int connect(int sockfd, const struct sockaddr* addr, socklen_t addr_len)
void syscall_connect(struct regs* r){
    
    save_context(r, current_process);
    

    int sockfd = (int )r->ebx;
    const struct sockaddr* addr = (const struct sockaddr* )r->ecx;
    socklen_t addr_len = (socklen_t )r->edx;

    file_t* file = &current_process->open_descriptors[sockfd];
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    
    if(fnode->flags  != FS_SOCKET){
        r->eax = -1;
        return;
    }


    struct socket* socket = fnode->device;
    struct sock* sock = socket->protocol_data;

    //already connected
    if(socket->state == SS_CONNECTED){
        r->eax = -1;
        return;
    }

    //if not then
    int err = socket->ops->connect(file, addr, addr_len);
    if(socket->state == SS_CONNECTING){

        current_process->syscall_state = SYSCALL_STATE_PENDING;
        current_process->syscall_number = r->eax;
        current_process->state = TASK_INTERRUPTIBLE;
        schedule(r);
        return;
    }

    r->eax = err;
    return;
};




void syscall_accept(struct regs* r){

    save_context(r, current_process);
    int sockfd = (int )r->ebx;
    struct sockaddr* addr = (struct sockaddr* )r->ecx;
    socklen_t* addrlen = (socklen_t* )r->edx;


    file_t* file = &current_process->open_descriptors[sockfd];
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    
    if(fnode->flags  != FS_SOCKET){
        r->eax = -1;
        return;
    }

    struct socket* socket = fnode->device;
    if(socket->state != SS_LISTENING){
        r->eax = -1;
        return;
    }

    int err = socket->ops->accept(file, addr, addrlen);

    if (err < 0)
    {
        if (err == -EAGAIN)
        {
            if (file->f_flags & O_NONBLOCK)
            {
                r->eax = err;
                return;
            }

            current_process->syscall_number = r->eax;
            current_process->syscall_state = SYSCALL_STATE_PENDING;
            current_process->state = TASK_INTERRUPTIBLE;
            schedule(r);
            return;
        }

        r->eax = err;
        return;
        
        
    }

    if(err > 0){
        //goooood
        file_t* file = &current_process->open_descriptors[err];
        fs_node_t* fnode = (fs_node_t*)file->f_inode;
        fnode->read = sock_node.read;
        fnode->write = sock_node.write;
        fnode->close = sock_node.close;
        
        r->eax = err;
        return;

    }    
};

void syscall_bind(struct regs* r){
    
    int fd = (int )r->ebx;
    struct sockaddr* addr = (struct sockaddr* )r->ecx;
    socklen_t addrlen = (socklen_t )r->edx;

    file_t* file = &current_process->open_descriptors[fd];
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    
    if(fnode->flags  != FS_SOCKET){
        r->eax = -8;
        return;
    }

    struct socket* socket = fnode->device;
    int err = socket->ops->bind(file, addr, addrlen);
    
    r->eax = err;
    return;
};

void syscall_listen(struct regs* r){
    
    int fd = r->ebx;
    int backlog = r->ecx;

    //check if fd points to a socket
    file_t* file = &current_process->open_descriptors[fd];
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    
    if(fnode->flags  != FS_SOCKET){
        r->eax = -1;
        return;
    }

    struct socket* socket = fnode->device;
    struct sock* sock = socket->protocol_data;

    socket->state = SS_LISTENING;
    sock->max_ack_backlog = backlog;
    
    r->eax = 0;
    return;
};

//ssize_t sendto(int sockfd, const void buf[.len], size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen);
void syscall_sendto(struct regs* r){
    int sockfd          =  (int )r->ebx;
    void* buf     =  (void* )r->ecx;
    size_t len          =  (size_t )r->edx;
    int flags           =  (int )r->esi;
    struct sockaddr *dest_addr =  (struct sockaddr *)r->edi;
    socklen_t addrlen   =  (socklen_t )r->ebp;


    if(sockfd < 0 || sockfd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file_t* file = &current_process->open_descriptors[sockfd];
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    
    if(!fnode){ //closed somehow?
        r->eax = -EBADF;
        return;
    }

    if(fnode->flags  != FS_SOCKET){
        r->eax = -EBADF;
        return;
    }

    struct socket* socket = fnode->device;
    struct sock* sock = socket->protocol_data;

    int err;
    err = socket->ops->sendto(file, buf, len, flags, dest_addr, addrlen);
    if(err < 0){

        if(err == -EAGAIN){
            if(file->f_flags & O_NONBLOCK){
                r->eax = err;
                return;
            }

            current_process->state = TASK_INTERRUPTIBLE;
            current_process->syscall_state = SYSCALL_STATE_PENDING;
            current_process->syscall_number = r->eax;

            save_context(r, current_process);
            schedule(r);
            return;

        }

        r->eax = err;
        return;

    }
    else{
        r->eax = (uint32_t)len;
        return;
    }

}

//int setsockopt(int sockfd, int level, int optname,const void* optval, socklen_t optlen);
void syscall_setsockopt(struct regs* r){

    int sockfd = (int)r->ebx;
    int level = (int)r->ecx;
    int optname = (int)r->edx;
    const void* optval = (const void*)r->esi;
    socklen_t optlen = (socklen_t)r->edi;

    file_t* file = &current_process->open_descriptors[sockfd];
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    
    if(fnode->flags  != FS_SOCKET){
        r->eax = -ENOTSOCK;
        return;
    }

    struct socket* socket = fnode->device;

    if(level == SOL_SOCKET){ //socket level ops
        switch(optname){
            case SO_BINDTODEVICE:
            ;
            struct nic* dev = netif_get_by_name((const char*)optval);
            if(!dev){
                r->eax = -EINVAL;
                return;
            }

            socket->bound_if = dev;
            r->eax = 0;
            break;

            default:
            r->eax = -EINVAL;
            break;
        }

    }
    


}


//ssize_t recvfrom(int sockfd, void buf[restrict .len], size_t len,
//int flags,
//struct sockaddr *_Nullable restrict src_addr,
//socklen_t *_Nullable restrict addrlen);

void syscall_recvfrom(struct regs* r){
    int sockfd = (int)r->ebx;
    void* buffer = (void*)r->ecx;
    size_t length = (size_t)r->edx;
    int flags = (int)r->esi;
    struct sockaddr * src = (struct sockaddr *)r->edi;
    socklen_t* addrlen = (socklen_t*)r->ebp;

    if(sockfd < 0  || sockfd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file_t* file = &current_process->open_descriptors[sockfd];
    fs_node_t* fnode = (fs_node_t*)file->f_inode;
    
    if(fnode->flags  != FS_SOCKET){
        r->eax = -ENOTSOCK;
        return;
    }

    struct socket* socket = fnode->device;
    


    struct iovec iov = {
        .iov_base = buffer,
        .iov_len = length
    };
    
    struct msghdr msg = {
        .msg_name = src,
        .msg_namelen = addrlen ? *addrlen : 0,
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = flags
    };
    

    int err = socket->ops->recvmsg(socket->file, &msg, 0);

    if(err < 0){
        if(err == -EAGAIN){
            if(file->f_flags & O_NONBLOCK){
                r->eax = err;
                return;
            }

            current_process->state = TASK_INTERRUPTIBLE;
            current_process->syscall_state = SYSCALL_STATE_PENDING;
            current_process->syscall_number = r->eax;

            save_context(r, current_process);
            schedule(r);
            return;   
        }

    }


    if(err >= 0 && src && addrlen){
        *addrlen = msg.msg_namelen;
    }

    r->eax = err;
    return;
}