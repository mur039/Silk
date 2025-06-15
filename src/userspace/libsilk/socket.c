#include <stddef.h>
#include <sys/socket.h>


int socket(int domain, int type, int protocol ){
    return syscall(SYSCALL_SOCKET, domain, type, protocol);
}

int listen(int fd, int backlog){
    return syscall(SYSCALL_LISTEN, fd, backlog);
}

int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen){
    return syscall(SYSCALL_BIND, sockfd, addr, addrlen);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addr_len){
    return syscall(SYSCALL_ACCEPT, sockfd, addr, addr_len);
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen){ 
    return syscall(SYSCALL_CONNECT, sockfd, addr, addrlen);
}

int sendto(int sockfd, void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen){
    return syscall(SYSCALL_SENDTO, sockfd, buf, len, flags, addr, addrlen);
}

int setsockopt(int sockfd, int level, int optname,const void* optval, socklen_t optlen){
    return syscall(SYSCALL_SETSOCKOPT, sockfd, level, optname, optval, optlen);
}

size_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr * restrict src_addr, socklen_t * restrict addrlen){
    return syscall(SYSCALL_RECVFROM, sockfd, buf, len, flags, src_addr, addrlen);
}