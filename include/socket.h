#ifndef __SOCKET__H__
#define __SOCKET__H__

#include <sys.h>
#include <g_list.h>
#include <pmm.h>
#include <circular_buffer.h>
#include <process.h>



#define EPHEMERAL_PORT_START 49152
#define EPHEMERAL_PORT_END 65535

//domains
#define AF_UNIX 0
#define AF_LOCAL AF_UNIX
#define AF_INET 1
#define AF_NETLINK 8

//socket types
#define SOCK_STREAM 0
#define SOCK_DGRAM 1 
#define SOCK_RAW 3

typedef unsigned int sa_family_t;
typedef size_t socklen_t;
struct sockaddr {
    sa_family_t     sa_family;      /* Address family */
    char            sa_data[];      /* Socket address */
};

struct sockaddr_in {
    sa_family_t sin_family;               /* AF_UNIX */
    uint16_t sin_port;
    uint32_t sin_addr;
};



struct proto_ops;
struct proto;
struct socket;
struct sock;

struct iovec {
    void   *iov_base;  /* Starting address */
    size_t  iov_len;   /* Size of the memory pointed to by iov_base. */
};

struct msghdr {
    void         *msg_name;       /* Optional address */
    socklen_t     msg_namelen;    /* Size of address */
    struct iovec *msg_iov;        /* Scatter/gather array */
    size_t        msg_iovlen;     /* # elements in msg_iov */
    void         *msg_control;    /* Ancillary data, see below */
    size_t        msg_controllen; /* Ancillary data buffer len */
    int           msg_flags;      /* Flags on received message */
};

struct proto_ops {
    int (*bind)(file_t *file, struct sockaddr * addr, socklen_t len);
    int (*connect)(file_t *file, const struct sockaddr* addr, socklen_t len);
    int (*accept)(file_t *file, struct sockaddr *addr, socklen_t *len);
    int (*sendmsg)(file_t *file, void *msg, size_t  len);
    int (*recvmsg)(file_t *file, struct msghdr* msg, int flags);
    int (*sendto)(file_t *file, void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen);
    short (*poll)(file_t *file, struct poll_table* pt);
  
};


struct proto {
    const char *name;

    //struct sock *(*create)(struct net *net, struct socket *sock, int protocol, int kern);
    void (*destroy)(struct sock *sk);

    int (*connect)(struct sock *sk, struct sockaddr *uaddr, int addr_len);
    int (*disconnect)(struct sock *sk, int flags);
    //int (*sendmsg)(struct sock *sk, struct msghdr *msg, size_t len);
    //int (*recvmsg)(struct sock *sk, struct msghdr *msg, size_t len, int flags);

    void (*close)(struct sock *sk, long timeout);
    
    // Memory alloc/free for sock
    struct sock *(*alloc)(struct proto *prot, int priority, int kern);
    void (*release_cb)(struct sock *sk);

    // Internal buffer and protocol-specific control
    int (*init)(struct sock *sk);
  
};


enum socket_state{
    SS_FREE = 0,
    SS_UNCONNECTED,
    SS_CONNECTING,
    SS_CONNECTED,
    SS_DISCONNECTING,
    SS_LISTENING
};
//abstract socket interface
struct socket{
    short           type;       // SOCK_STREAM, SOCK_DGRAM, etc.
    unsigned long   flags;      //non-blocking, async etc
    unsigned long   state;      // SS_FREE SS_UNCONNECTED SS_CONNECTING SS_CONNECTED SS_DISCONNECTING: Closing process.

    const struct proto_ops *ops; // Function pointers for socket ops, inet, unix

    void    *protocol_data; //// Protocol-specific data
    struct socket *conn; //good
    struct socket *iconn; //incomplete connected, before accept finalizes them
    struct socket *next; //allows sockets to be chained, accepts queue, uds connection list etc
    struct nic* bound_if; //bound interface
    //we don't have inodes darling :/
    //as well as async :/
    file_t*     file;      // File representation (fd)

};


//state machine lives in here
struct sock {
    struct socket *sk_socket; // Back-reference
    const struct proto* sk_prot;
    size_t sk_refcount;
    pid_t sk_proc;

    unsigned char ack_backlog;
    unsigned char max_ack_backlog;
    //internal buffer as well
    // circular_buffer_t recv;
    struct sk_buff *rx_head;
    struct sk_buff *rx_tail;
    list_t rx_waitqueue;
    list_t accept_waitqueue;

    unsigned long state; //like TCP's state or sumthin
    
    list_t pending_writes; //for both udp and tcp, waiting until
    list_t pending_reads; //for tcp
    
    
};

#define SOL_SOCKET 0
#define SO_BINDTODEVICE 1

void initialize_sockets(int max_socket_count);
struct socket* socket_allocate_socket();

void syscall_socket(struct regs* r);
void syscall_connect(struct regs* r);
void syscall_accept(struct regs* r);
void syscall_bind(struct regs* r);
void syscall_listen(struct regs* r);
void syscall_sendto(struct regs* r);
void syscall_setsockopt(struct regs* r);
void syscall_recvfrom(struct regs* r);
short socket_poll(struct fs_node *fn, struct poll_table* pt);


#endif