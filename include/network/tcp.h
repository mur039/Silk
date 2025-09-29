#ifndef __TCP_H
#define __TCP_H
#include <stddef.h>
#include <network/ipv4.h>

/*
0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                            TCP Header Format

*/

//TCP control bits
#define URG  1 << 5 //Urgent Pointer field significant
#define ACK  1 << 4 //Acknowledgment field significant
#define PSH  1 << 3 //Push Function
#define RST  1 << 2 //Reset the connection
#define SYN  1 << 1 //Synchronize sequence numbers
#define FIN  1 << 0  //No more data from sender




struct tcp {
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t seq_numb;
    uint32_t ack_numb;

    uint16_t data_off_rsrvd_ctrl; // 4;6;6;

    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
    uint32_t options:24; // 24-bit !!!
    uint8_t _padding;
    uint8_t data[];


} __attribute__((packed));


struct tcp_pseudo_header {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t  zero;
    uint8_t  protocol; // 6 for TCP
    uint16_t tcp_len;
} __attribute__((packed));


enum tcp_states {
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYNSENT,
    TCP_SYNRECEIVED,
    TCP_ESTABLISHED,
    TCP_FINWAIT1,
    TCP_FINWAIT2,
    TCP_CLOSEWAIT,
    TCP_CLOSING,
    TCP_LASTACK,
    TCP_TIMEWAIT,
};

struct tcp_sock {
    struct inet_sock isk;
    enum tcp_states tcp_state;
    uint32_t rcv_next;
    uint32_t snd_next;
    uint32_t snd_una;
    
};

int tcp_input(struct sk_buff* skb);
int tcp_create_socket(struct socket* socket);
uint16_t tcp_package_calc_checksum(struct tcp* package, size_t len, uint32_t src_ip, uint32_t dst_ip);
#endif