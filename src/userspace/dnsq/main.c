#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <termios.h>
#include <string.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#define DNS_SERVER_PORT 53
struct dns_pack{
    uint16_t transaciton_id;
    uint16_t flags;
    uint16_t no_q;
    uint16_t no_a;
    uint16_t no_auth_rr;
    uint16_t no_addi_rr;
};


struct dns_rr {
    uint16_t cname_ptr;      // usually a pointer to the question's name
    uint16_t type;          // e.g., A=1, AAAA=28, CNAME=5
    uint16_t class;         // IN = 1
    uint32_t ttl;
    uint16_t rdlength;      // length of RDATA
    uint8_t  rdata[];       // the actual IP, domain name, etc.
};


int main(int argc, char** argv){
    
    int fd; 
    int err;

    if(argc < 2){
        puts("Expected domain name\n");
        return 1;
    }

    err = socket(AF_INET, SOCK_DGRAM, 0);
    if(err < 0){
        puts("[-]Failed to allocate socket\n");
        return 1;
    }

    fd = err;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    // addr.sin_port = htons(1234),
    // addr.sin_addr = 0; //listen at all interfaces ,wwe don't have one but anyway

    // err = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    // if(err < 0){
    //     printf("Failed to bind socket\n");
    //     return 1;
    // }
    // puts("[+]Binded\n");

    //reuse reduce
    addr.sin_port = htons(DNS_SERVER_PORT);
    addr.sin_addr = htonl(0x0a000203); //10.0.2.3 ? 
    addr.sin_addr = htonl(0x08080808); //8.8.8.8 ? 

    
    int query_len;
    struct dns_pack* dns = calloc(1, 512);
    uint8_t* byteptr = (uint8_t*)dns;

    // 1. DNS Header (12 bytes)
    byteptr[0] = 0x12; // ID high byte
    byteptr[1] = 0x34; // ID low byte
    byteptr[2] = 0x01; // flags: recursion desired
    byteptr[3] = 0x00;
    byteptr[4] = 0x00;
    byteptr[5] = 0x01; // QDCOUNT: 1
    byteptr[6] = 0x00;
    byteptr[7] = 0x00; // ANCOUNT: 0
    byteptr[8] = 0x00;
    byteptr[9] = 0x00; // NSCOUNT: 0
    byteptr[10] = 0x00;
    byteptr[11] = 0x00; // ARCOUNT: 0
    query_len = 12;

    // 2. Question: "example.com" in DNS format
    const char *domain = argv[1];


    const char *start = domain;
    const char *end = domain;
    while (*end){
        while (*end && *end != '.')
            end++;
        int len = end - start;
        byteptr[query_len++] = len;
        memcpy(&byteptr[query_len], start, len);
        query_len += len;
        if (*end == '.')
            end++;
        start = end;
    }

    byteptr[query_len++] = 0x00; // end of name

    // 3. QTYPE = 1 (A record)
    byteptr[query_len++] = 0x00;
    byteptr[query_len++] = 0x01;

    // 4. QCLASS = 1 (IN)
    byteptr[query_len++] = 0x00;
    byteptr[query_len++] = 0x01;

    err = sendto(fd, byteptr, query_len, 0, (struct sockaddr*)&addr, sizeof(addr));
    
    if(err < 0){
        printf("[-]sendto failed with %d\n", err);
        return 1;
    }
    
    socklen_t socklen;
    err = recvfrom(fd, byteptr, 512, 0, (struct sockaddr*)&addr, &socklen);
    if(err < 0){
        printf("[-]recvfrom failed with %d\n", err);
        return 1;
    }

    
    printf("received %u\n", err);
    printf("Transaciton ID: %x\n",      ntohs(dns->transaciton_id));
    printf("flags: %x\n",               ntohs(dns->flags));
    printf("Number of questions: %u\n", ntohs(dns->no_q));
    printf("Number of answers: %u\n",   ntohs(dns->no_a));
    printf("Number of authrr: %u\n",    ntohs(dns->no_auth_rr));
    printf("Number of addirr: %u\n",    ntohs(dns->no_addi_rr));
    
    byteptr += 12; //pas over the header
    printf("qname: %s\n", byteptr);
    while(*byteptr) byteptr++; //passover qname?
    byteptr++; //pass over the null-terminator

    byteptr += 4; //qtype and qclass

    byteptr += 2; //name a pointer?
    uint16_t* wordptr = (uint16_t*)byteptr;
    
    int answercount = ntohs(dns->no_a);

    int answerlen = 0;
    printf("answer type: %u\n", ntohs(*(uint16_t *)byteptr));
    byteptr += 2; // must be 1
    printf("answer class: %u\n", ntohs(*(uint16_t *)byteptr));
    byteptr += 2; // must be 1
    printf("answer ttl: %u\n", ntohl(*(uint32_t *)byteptr));
    byteptr += 4;
    answerlen = ntohs(*(uint16_t *)byteptr);
    printf("answer rdlen: %u\n", ntohs(*(uint16_t *)byteptr));
    byteptr += 2;

    printf(
        "%u.%u.%u.%u it is\n",
        byteptr[0],
        byteptr[1],
        byteptr[2],
        byteptr[3]);

    return 0;    
}
