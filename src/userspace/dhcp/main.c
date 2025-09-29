#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

#include <sys/socket.h>
#include <stdint.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

 	
/*OPCODES*/
#define DHCPDISCOVER           1
#define DHCPOFFER              2
#define DHCPREQUEST            3
#define DHCPDECLINE            4
#define DHCPACK                5
#define DHCPNAK                6
#define DHCPRELEASE            7
#define DHCPINFORM             8
#define DHCPFORCERENEW         9
#define DHCPLEASEQUERY        10
#define DHCPLEASEUNASSIGNED   11
#define DHCPLEASEUNKNOWN      12
#define DHCPLEASEACTIVE       13
#define DHCPBULKLEASEQUERY    14
#define DHCPLEASEQUERYDONE    15
#define DHCPACTIVELEASEQUERY  16
#define DHCPLEASEQUERYSTATUS  17
#define DHCPTLS               18

struct dhcp_packet {                
    uint8_t op;       // 1=request, 2=reply
    uint8_t htype;    // Hardware type (1 for Ethernet)
    uint8_t hlen;     // Hardware address length (6 for MAC)
    uint8_t hops;     // Set to 0
    uint32_t xid;     // Transaction ID
    uint16_t secs;    // Seconds elapsed
    uint16_t flags;   // 0x8000 = broadcast
    uint32_t ciaddr;  // Client IP (if already assigned)
    uint32_t yiaddr;  // "Your" IP (set by server)
    uint32_t siaddr;  // Next server IP
    uint32_t giaddr;  // Relay agent IP
    uint8_t chaddr[16]; // Client hardware address
    uint8_t sname[64];  // Optional server host name
    uint8_t file[128];  // Boot file name
    uint8_t options[]; // DHCP options (variable length)
};

#define DHCP_TRANSACTION_ID 0x3903F326
#define DHCP_MAGIC_COOKIE 0x63825363


int dhcp_construct_discovery(void* buffer, void* macaddr){
    
    struct dhcp_packet* dhcp = buffer;
    memset(buffer, 0, 260);
    //construct an dhcp message

    dhcp->op = DHCPDISCOVER;
    dhcp->htype = 1;  //we are using ethernet
    dhcp->hlen = 6; //for macaddr
    dhcp->hops = 0;
    dhcp->xid = htonl(DHCP_TRANSACTION_ID);
    dhcp->secs = 0;
    dhcp->flags = 0; //htons(0x8000); //for discovery?

    memcpy(dhcp->chaddr, macaddr, 6);
    uint32_t* dwordptr = (uint32_t*)dhcp->options;
    *dwordptr = htonl(DHCP_MAGIC_COOKIE);

    return 264;
}

int dhcp_construct_request(void* buffer, void* macaddr, void* ipaddr){
    struct dhcp_packet* dhcp = buffer;
    memset(buffer, 0, 260);
    //construct an dhcp message

    dhcp->op = 1;
    dhcp->htype = 1;  //we are using ethernet
    dhcp->hlen = 6; //for macaddr
    dhcp->hops = 0;
    dhcp->xid = htonl(DHCP_TRANSACTION_ID);
    dhcp->secs = 0;
    dhcp->flags = 0; //htons(0x8000); //for discovery?

    dhcp->siaddr = htonl(0xc0a80101);
    memcpy(dhcp->chaddr, macaddr, 6);
    uint32_t* dwordptr = (uint32_t*)dhcp->options;
    *dwordptr = htonl(DHCP_MAGIC_COOKIE);

    return 264;
}

int if_get_interfaces(int sockfd ,struct ifconf* ifc){
    
    if( ioctl(sockfd, SIOCGIFCONF, ifc) == -1){
        printf("[-]failed to ioctl");
        return -1;
    }

    struct ifreq* ifr = ifc->ifc_req;
    int n_interfaces = ifc->ifc_len / sizeof(struct ifreq);

    return n_interfaces;
}


uint8_t buffer[512]; //general purpose buffer

int main( int argc, char* argv[]){

    if(argc != 2){
        printf("Usage: %s  [interface name]\n", argv[0]);
        return 1;
    }
    
    const char* if_name = argv[1];

    int err;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0){
        puts("Failed to allocate a socket\n");
        return 1;
    }    


    struct ifconf ifc = {.ifc_len = 512, .ifc_buf = buffer};
    int ifcount = if_get_interfaces(fd, &ifc);
    struct ifreq* ifr = ifc.ifc_req;

    int index = -1;
    for(int i = 0; i < ifcount; ++i){
        printf("Interface name : %s \n", ifr[i].ifr_name);
        if(!strcmp(if_name, ifr[i].ifr_name) ){
            index = i;
            break;
        }
    }

    if(index < 0){
        printf("No such interface \"%s\"\n", if_name);
        return 1;
    }


    err = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, if_name, strlen(if_name)); //bind that interface to our socket

    //soo i need the mac addr
    if( ioctl(fd, SIOCGIFHWADDR, &ifr[index]) == -1){
        printf("[-] retrieving hardware address failed\n");
        return 1;
    }

    uint8_t ifhwaddr[6];
    memcpy(ifhwaddr, ifr[index].ifr_hwaddr.sa_data, 6);


    //bind a socket to listen from all interfaces
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DHCP_CLIENT_PORT),
    addr.sin_addr = 0; // listen at all interfaces ,wwe don't have one but anyway

    err = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(err < 0){
        printf("[-] Failed to bind socket\n");
        return 1;
    }
    puts("[+]Binded\n");

    
    
    //set socket fd as non-blocking (Should've implemented poll or select)
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    //set up buffer for receiving
    memset(buffer, 0, sizeof(buffer));

    //udp multicast
    struct sockaddr_in target = {
        .sin_family = AF_INET,
        .sin_port = htons(DHCP_SERVER_PORT),
        .sin_addr = htonl(0xFFFFFFFF)
    };

    enum client_state {
        CLIENT_OFFER,
        CLIENT_ACK,
        CLIENT_NACK
    };

    int state;
    socklen_t socklen;
    struct dhcp_packet* dhcp = (void*)buffer;
    err = dhcp_construct_discovery(dhcp, ifhwaddr);
    if(err < 0){
        printf("Failed to construct discovery message\n");
        return 1;
    }

    
    uint32_t offerip = 0;
    uint32_t serverip = 0;
    uint32_t dnsip = 0; 
    
    state = CLIENT_OFFER;
    while(1){

        switch(state){
            case CLIENT_OFFER:
                dhcp_construct_discovery(dhcp, ifhwaddr);
                sendto(fd, dhcp, err, 0, (struct sockaddr*)&target, sizeof(target));

                for(int i = 0; i < 0xFFFFF; ++i); //timerless timeout :/
                err = recvfrom(fd, buffer, 300, 0, (struct sockaddr*)&addr, &socklen);
                if(err < 0){ //probablt -EGAIN
                    puts("Waiting for offer, timeout reached defaulting to 192.168.100.3\n");

                    if_get_interfaces(fd, &ifc); //restore it

                    uint32_t laddr = 0xc0a86403;
                    uint32_t gaddr = 0xc0a86401;
                    

                    ifr[index].ifr_addr = htonl(laddr);
                    ifr[index].ifr_map.mem_end = htonl(gaddr); // :/
                    ioctl(fd, SIOCSIFADDR, &ifr[index]);
                    return 1;
                }

                uint32_t refval;
                //validate xid, magic cookie
                refval = htonl(DHCP_TRANSACTION_ID);
                if(dhcp->xid != refval){
                    printf("Invalid XID retrying\n");
                    continue;
                }

                refval = htonl(DHCP_MAGIC_COOKIE);
                if( *(uint32_t*)dhcp->options != refval ){
                    printf("Invalid magic cookie retrying\n");
                    continue;
                }


                //we received an offer
                if(dhcp->op != DHCPOFFER){
                    printf("expecting offer, got %u\n", dhcp->op);
                    continue;
                }

                offerip = dhcp->yiaddr;
                serverip = dhcp->siaddr;

                uint8_t* byteptr = (uint8_t*)&offerip;
                printf("Offered IP, %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3]);
                
                byteptr = (uint8_t*)&serverip;
                printf("Server IP, %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3]);

                uint8_t* options = dhcp->options;
                options += 4; //passing over magic cookie

                while(*options != 0xFF && options < (buffer + err)){
                    int code = *(options++);
                    int length = *(options++);
                    
                    if(!code) 
                        continue; //no length field for code 0

                    printf("code:%u, length:%u -> ", code, length );
                    
                    
                    if(code == 51){
                        printf("-> offered lease time for this ip address: %u seconds", ntohl(*(uint32_t*)options));
                    }else if(code == 6){
                        byteptr = options;
                        printf("DNS IP, %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3]);
                        dnsip = *(uint32_t*)byteptr;
                    }
                    
                    putchar('\n');

                    options += length;
                }

                state = CLIENT_ACK;

            break;
            case CLIENT_ACK:
                err = dhcp_construct_request(dhcp, ifhwaddr, &offerip);
                sendto(fd, dhcp, err, 0, (struct sockaddr*)&target, sizeof(target));

                for(int i = 0; i < 0xFFFFF; ++i); //timerless timeout :/
                err = recvfrom(fd, buffer, 300, 0, (struct sockaddr*)&addr, &socklen);
                if(err < 0){
                    printf("Waiting on the acki timeout...");
                    return 1;
                }

                //buffer is shared thorugh out the program and now it is clubbered
                if_get_interfaces(fd, &ifc); //restore it

                
                printf("before setting ip and gateway addr, offeredip:%x , serverip :%x\n", offerip, serverip);
                ifr[index].ifr_addr = (offerip);
                ifr[index].ifr_map.mem_end = (serverip);

                
                

                ioctl(fd, SIOCSIFADDR, &ifr[index]); //set the ip address
                

                state = -1;
                    
            break;
            case CLIENT_NACK:
                return 2;
            break;
        }

        if(state < 0) break;
    }




    chdir("/etc/");
    int resolvfd = open("resolv.conf", O_WRONLY | O_CREAT);
    if(fd < 0){
        printf("failed to create /etc/resolv.conf\n");
        return 0;
    }

    if(dnsip){
        //output to the /etc/resolv.conf        
        uint8_t* byteptr = (uint8_t*)&dnsip;
        sprintf(buffer, "nameserver %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3]);
        write(resolvfd, buffer, strlen(buffer));
    }

    write(resolvfd, "nameserver 8.8.8.8\n", 19);
    
    close(resolvfd);

    return 0;
}






int _main( int argc, char* argv[]){

    //check the argv here 
    if(argc != 2){
        printf("Usage: %s  [interface name]\n", argv[0]);
        return 1;
    }

    const char* if_name = argv[1];

    int err;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0){
        puts("Failed to allocate a socket\n");
        return 1;
    }    
    
    //we can query the interfaces here
    uint8_t buffer[512];
    struct ifconf ifc = {.ifc_len = 512, .ifc_buf = buffer};
    
    if( ioctl(fd, SIOCGIFCONF, &ifc) == -1){
        printf("[-]failed to ioctl");
        return 1;
    }
    
    struct ifreq* ifr = ifc.ifc_req;
    int n_interfaces = ifc.ifc_len / sizeof(struct ifreq);

    int index = -1;
    for(int i = 0; i < n_interfaces; ++i){
        printf("Interface name : %s \n", ifr[i].ifr_name);
        if(!strcmp(if_name, ifr[i].ifr_name) ){
            index = i;
            break;
        }
    }

    if(index < 0){
        printf("No such interface, %s\n", if_name);
        return 1;
    }

    //soo i need the mac addr
    if( ioctl(fd, SIOCGIFHWADDR, &ifr[index]) == -1){
        printf("[-] retrieving hardware address failed\n");
        return 1;
    }

    //i know that inteface exist so i can just 
    err = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, if_name, strlen(if_name));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DHCP_CLIENT_PORT),
    addr.sin_addr = 0; // listen at all interfaces ,wwe don't have one but anyway

    err = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(err < 0){
        printf("[-] Failed to bind socket\n");
        return 1;
    }
    puts("[+]Binded\n");


    struct sockaddr_in target = {
        .sin_family = AF_INET,
        .sin_port = htons(DHCP_SERVER_PORT),
        .sin_addr = htonl(0xFFFFFFFF)
    };
    
    //construct an dhcp message
    struct dhcp_packet* dhcp = calloc(1, 300);
    dhcp->op = 1;
    dhcp->htype = 1;
    dhcp->hlen = 6; //for macaddr
    dhcp->hops = 0;
    dhcp->xid = htonl(DHCP_TRANSACTION_ID);
    dhcp->secs = 0;
    dhcp->flags = htons(0x8000); //for discovery?

    memcpy(dhcp->chaddr, ifr[index].ifr_hwaddr.sa_data, 6);
    uint32_t* dwordptr = (uint32_t*)dhcp->options;
    *dwordptr = htonl(DHCP_MAGIC_COOKIE);
    
    //discovert
    err = sendto(fd, dhcp, 264, 0, (struct sockaddr*)&target, sizeof(target));
    

    //after that we should hear an offer from dhcp server
    //so read atleast 240 bytes of it

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);


    //offer
    socklen_t socklen;
    for(int i = 0; i < 0xFFFFF; ++i);
    err = recvfrom(fd, dhcp, sizeof(struct dhcp_packet) + 4, 0, (struct sockaddr*)&addr, &socklen);

    if(err != 240){
        printf("I had enough bitch\n");
        uint32_t* dwordptr = (uint32_t*)&ifr[index].ifr_addr; 
        uint8_t* byteptr =  (uint8_t*)&ifr[index].ifr_addr;
        
        byteptr[3] = 192;
        byteptr[2] = 168;
        byteptr[1] = 100;
        byteptr[0] = 3;
        *dwordptr = htonl(*dwordptr);
        
        ioctl(fd, SIOCSIFADDR, &ifr[index]);
        return 1;
    }
    
        
    printf(
        "op: %u\n"
        "htype: %u\n"
        "hlen: %u\n"
        "xid: %x    \n"
        "flags: %x\n"
        "ciaddr: %x\n"
        "yiaddr: %x\n"
        "siaddr: %x\n"
        "giaddr: %x\n"
        "magic_cookie: %x\n"
        ,
        dhcp->op,
        dhcp->htype,
        dhcp->hlen,
        ntohl(dhcp->xid),
        dhcp->flags,
        dhcp->ciaddr,
        dhcp->yiaddr,
        dhcp->siaddr,
        dhcp->giaddr,
        ntohl(*(uint32_t*)dhcp->options)
    );

    uint32_t offered_ip = (dhcp->yiaddr);
    uint32_t server_ip = (dhcp->siaddr);


    uint8_t* byteptr = (uint8_t*)&offered_ip;   
    dwordptr = (uint32_t*)byteptr;
    printf("IP offered by the dhcp server is : %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3] );
    byteptr = (uint8_t*)&server_ip;
    printf("and server ip that is : %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3] );

    //also read options until an 0xff is found


    //request
    memset(dhcp, 0, 300);
    
    dhcp->op = 1;
    dhcp->htype = 1;
    dhcp->hlen = 6;
    dhcp->hops = 0;

    dhcp->xid = htonl(DHCP_TRANSACTION_ID);
    
    dhcp->secs = 0;
    dhcp->flags = 0; //for discovery?
    
    dhcp->siaddr = htonl(0xc0a80101);
    dhcp->giaddr = 0;

    memcpy(dhcp->chaddr, ifr[index].ifr_hwaddr.sa_data, 6);

    dwordptr = (uint32_t*)dhcp->options;
    *dwordptr = htonl(DHCP_MAGIC_COOKIE);
    
    err = sendto(fd, dhcp, 264, 0, (struct sockaddr*)&target, sizeof(target));
    
    //ack
    err = recvfrom(fd, dhcp, sizeof(struct dhcp_packet) + 4, 0, (struct sockaddr*)&addr, &socklen);


    if(err < 0)
        return 1;
    
    printf(
        "op: %u\n"
        "htype: %u\n"
        "hlen: %u\n"
        "xid: %x    \n"
        "flags: %x\n"
        "ciaddr: %x\n"
        "yiaddr: %x\n"
        "siaddr: %x\n"
        "giaddr: %x\n"
        "magic_cookie: %x\n"
        ,
        dhcp->op,
        dhcp->htype,
        dhcp->hlen,
        ntohl(dhcp->xid),
        dhcp->flags,
        dhcp->ciaddr,
        dhcp->yiaddr,
        dhcp->siaddr,
        dhcp->giaddr,
        ntohl(*(uint32_t*)dhcp->options)
    );
    
    offered_ip = (dhcp->yiaddr);
    server_ip = (dhcp->siaddr);


    byteptr = (uint8_t*)&offered_ip;   
    dwordptr = (uint32_t*)byteptr;
    
    printf("IP offered by the dhcp server is : %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3] );
    byteptr = (uint8_t*)&server_ip;
    printf("and server ip that is : %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3] );

    dwordptr = (uint32_t*)&ifr[index].ifr_addr; 
    *dwordptr = htonl(*(uint32_t*)byteptr);
    
    ioctl(fd, SIOCSIFADDR, &ifr[index]);


    byteptr = (uint8_t*)&offered_ip;
    chdir("/etc/");
    int resolvfd = open("resolv.conf", O_WRONLY | O_CREAT);
    if(fd < 0){
        printf("failed to create /etc/resolv.conf\n");
        return 0;
    }

    sprintf(buffer, "nameserver %u.%u.%u.%u\n", byteptr[0], byteptr[1], byteptr[2], byteptr[3]);
    write(resolvfd, buffer, strlen(buffer));
    close(resolvfd);

    free(dhcp);
    
    return 0;
}

