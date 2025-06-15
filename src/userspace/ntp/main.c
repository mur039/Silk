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


#define NTP_TIMESTAMP_DELTA 2208988800ull


#include <stdio.h>
#include <stdint.h>

const int days_in_month[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

int is_leap(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

void unix_to_utc(uint32_t epoch, int* year, int* month, int* day,
                 int* hour, int* minute, int* second) {
    uint32_t days = epoch / 86400;
    uint32_t secs = epoch % 86400;

    *hour = secs / 3600;
    secs %= 3600;
    *minute = secs / 60;
    *second = secs % 60;

    int y = 1970;
    while (1) {
        int days_in_year = is_leap(y) ? 366 : 365;
        if (days >= days_in_year) {
            days -= days_in_year;
            y++;
        } else {
            break;
        }
    }

    *year = y;
    int m = 0;
    while (1) {
        int dim = days_in_month[m];
        if (m == 1 && is_leap(y)) dim++; // February leap year

        if (days >= dim) {
            days -= dim;
            m++;
        } else {
            break;
        }
    }

    *month = m + 1;
    *day = days + 1;
}

void format_unix_time(uint32_t epoch, char* buf) {
    int year, month, day, hour, minute, second;
    unix_to_utc(epoch, &year, &month, &day, &hour, &minute, &second);
    sprintf(buf, "%d-%d-%d %d:%d:%d UTC",
            year, month, day, hour, minute, second);
}


int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    unsigned char packet[48] = {0};

    // Set LI = 0, VN = 3, Mode = 3 (client)
    packet[0] = 0x1B;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // NTP server address (Google NTP)
    const char* ntp_ip = "216.239.35.0"; // time.google.com
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(123);
    server_addr.sin_addr  = 0;
    
    uint8_t* byteptr = (uint8_t*)&server_addr.sin_addr;
    byteptr[0] = 216;
    byteptr[1] = 239;
    byteptr[2] = 35;
    byteptr[3] = 0;


    // Send packet
    if (sendto(sockfd, packet, sizeof(packet), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        return 1;
    }

    // Receive packet
    socklen_t addr_len = sizeof(server_addr);
    if (recvfrom(sockfd, packet, sizeof(packet), 0,
                 (struct sockaddr*)&server_addr, &addr_len) < 0) {
        perror("recvfrom");
        close(sockfd);
        return 1;
    }

    close(sockfd);

    // Extract the transmit timestamp (bytes 40-43)
    uint32_t transmit_time;
    memcpy(&transmit_time, &packet[40], 4);
    transmit_time = ntohl(transmit_time);

    time_t unix_time = (time_t)(transmit_time - NTP_TIMESTAMP_DELTA);
    printf("Unix epoch: %u\n", unix_time);

    char buffer[128];
    format_unix_time(unix_time, buffer);
    printf("%s\n", buffer);

    return 0;

}

























int _main(int argc, char** argv){
    
    int fd; 
    int err;


    err = socket(AF_INET, SOCK_DGRAM, 0);
    if(err < 0){
        puts("[-]Failed to allocate socket\n");
        return 1;
    }

    fd = err;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;

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
    const char *domain = "time.google.com";


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

    printf("answer type: %u\n", ntohs(*(uint16_t*)byteptr));  byteptr += 2; //must be 1
    printf("answer class: %u\n", ntohs(*(uint16_t*)byteptr)); byteptr += 2;  //must be 1
    printf("answer ttl: %u\n", ntohl(*(uint32_t*)byteptr));  byteptr += 4; 
    printf("answer rdlen: %u\n", ntohs(*(uint16_t*)byteptr));  byteptr += 2; 



    //we should have time.google.com
    uint32_t ntpsip = *(uint32_t*)byteptr;
    addr.sin_port = htons(DNS_SERVER_PORT);
    addr.sin_addr = ntpsip; //10.0.2.3 ? 
    
    
    return 0;    
}





