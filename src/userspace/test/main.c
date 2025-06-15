
#include <stdint-gcc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

int puts(const char *str){
    return write(FILENO_STDOUT, str, strlen(str));
}

int putchar(int ch){
    return write(FILENO_STDOUT, &ch, 1);
}
typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;


typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
} pixel_t;

enum server_bytecodes{
    SERV_REQUEST = 0,
    SERV_REPLY,
    SERV_EVENT,

    SERV_HEADER = 0x69

};

enum server_requesnt{
    CLIENT_CREATE_WINDOW = 1,
    CLIENT_SET_WINDOW_SIZE,
    CLIENT_DESTROY_WINDOW,
    CLIENT_ASCII_STRING


};


struct ds_packet{
    uint8_t header;
    uint8_t type;
    uint8_t length;
    
    uint8_t opt0;
    uint8_t opt1;
    uint8_t opt2;
    uint8_t opt3;
    uint8_t opt4;
    uint8_t opt5;
    uint8_t opt6;
    uint8_t opt7;
    uint8_t opt8;
    uint8_t opt9;
    uint8_t opt10;
    uint8_t opt11;
    uint8_t opt12;
};

void send_server_message(int sock, const struct ds_packet packet){
  
    write(sock, &packet, packet.length);
}

void mds_create_window(int fd){
    struct ds_packet package;
    package.header = SERV_HEADER;
    package.type = SERV_REQUEST;
    package.length = 4;
    package.opt0 = CLIENT_CREATE_WINDOW ;

    send_server_message(fd, package);

}

mds_set_window_size(int fd, int width, int height){
    struct ds_packet package;
    package.header = SERV_HEADER;
    package.type = SERV_REQUEST;
    package.opt0 = CLIENT_SET_WINDOW_SIZE;
    package.opt1= width;
    package.opt2 = height;
    package.length = 6;

    
    send_server_message(fd, package);
}

mds_destroy_window(int fd){
    struct ds_packet package;
    package.header = SERV_HEADER;
    package.type = SERV_REQUEST;
    package.opt0 = CLIENT_DESTROY_WINDOW;
    
    package.length = 4;
    
    send_server_message(fd, package);
}

//str, max 10 character
mds_create_string(int fd, int x, int y, char * str){
    struct ds_packet package;
    package.header = SERV_HEADER;
    package.type = SERV_REQUEST;
    package.opt0 = CLIENT_ASCII_STRING;
    package.opt1 = x;
    package.opt2 = y;
    memcpy(&package.opt3, str, 10);
    
    
    package.length = 16;
    
    send_server_message(fd, package);
}


typedef struct {
    uint16_t       vendor_id; uint16_t device_id;
    uint16_t     command_reg; uint16_t status;
    uint8_t      revision_id; uint8_t programming_if; uint8_t  subclass; uint8_t class_code;
    uint8_t  cache_line_size; uint8_t  latency_timer; uint8_t  header_type; uint8_t built_in_self_test;

}pci_common_header_t;

typedef struct {
    uint32_t base_address_0;
    uint32_t base_address_1;
    uint32_t base_address_2;
    uint32_t base_address_3;
    uint32_t base_address_4;
    uint32_t base_address_5;
    uint32_t card_bus_cis_pointer;
    uint16_t sub_system_id; uint16_t sub_system_vendor_id;
    uint32_t expansion_rom_base_address;
    uint8_t capabilities_pntr; uint8_t _reserved[3];
    uint32_t __reserved;
    uint8_t  interrupt_line; uint8_t interrupt_pin; uint8_t min_grant; uint8_t max_latency;

} pci_type_0_t;



int getchar(){
    uint8_t ch = 0;
    int ret = read(FILENO_STDIN, &ch, 1);

    if(ret == -EAGAIN) return -1;
    else if(ret > 0) return ch;
}


int main( int argc, char* argv[]){

    malloc_init(); //to get dynamic memory baby
    

    int err;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0){
        puts("Failed to allocate a socket\n");
        return 1;
        }    
        puts("[+]Created a socket\n");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0x80);
    
    addr.sin_addr = 0; // listen at all interfaces ,we don't have many but one, anyway
    uint8_t* byteptr = &addr.sin_addr;
    byteptr[0] = 192;
    byteptr[1] = 168;
    byteptr[2] = 100;
    byteptr[3] = 3;
    addr.sin_addr = ntohl(addr.sin_addr);
    //no listen on our localaddr
    socklen_t socklen = sizeof(addr);
    

    err = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(err < 0){
        printf("Failed to bind socket\n");
        return 1;
        }
        puts("[+]Binded\n");
    
        
    char characters[128];
    while (1){

        //basically udp echo server
        err = recvfrom(fd, characters, 128, 0, &addr, &socklen);//read(fd, &character, 1);
        if (err){
            characters[err] = '\0';
            uint8_t* byteptr = &addr.sin_addr;
            uint16_t port = ntohs(addr.sin_port);
            printf(
                "from %u.%u.%u.%u:%u received: %s%c",
                byteptr[0], byteptr[1], byteptr[2], byteptr[3],
                port,
                characters,
                characters[strlen(characters) - 1] == '\n' ? '\0' : '\n'
            );
            //echo it back
            sendto(fd, characters, err, 0, &addr, sizeof(addr));
        }
        else if (err == 0){ //??
            return 0;
        }
        else{
            printf("returned error: %u\n", err);
            return 1;
        }
    }
    return 0;

    /*

int servfd = socket(AF_UNIX, SOCK_RAW, 0);
if(servfd < 0){
puts("Failed to allocate a socket\n");
return 1;
}

struct sockaddr_un nix;
nix.sun_family = AF_UNIX;
sprintf(nix.sun_path, "mds");


printf("[client] created and socket\n");

err = connect(servfd, (struct sockaddr*)&nix, sizeof(nix));
if(err < 0){
printf("[client]Failed connect addr, err=%x\n", err);
return 1;
}

puts("[client] connected to server\n");

mds_create_window(servfd);

puts("[client] sent \"created a window\" to server\n");
puts("[client] waiting for keypress to destroy and close the connection\n");

int ch;
ch = getchar();
printf("return value of getchar %x:%c\n", ch, ch);


mds_destroy_window(servfd);
puts("[client] sent \"destroy the window\" to server\n");

close(servfd);
return 0;
*/
}
