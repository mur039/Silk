
#include <stdint-gcc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int puts(const char *str){
    return write(0, str, strlen(str));
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
  for(int i = 0; i < packet.length; ++i){
    uint8_t * head = &packet;
    write(sock, &head[i], 1);
  }
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






enum port_ioctl_request{
    PORTIO_CAPABILITY = 0, //not implemented
    PORTIO_READ,
    PORTIO_WRITE,
};

#define PEX_CONNECT 2

int main( int argc, char* argv[]){

    malloc_init(); //to get dynamic memory baby

    int pexfd = open("/dev/pex0", O_RDWR);
    if(pexfd < 0){
        puts("Failed to open /dev/pex0\n");
        return 1;
    }

    int result;

    
    //for now let's bind again
    result = ioctl(pexfd, PEX_CONNECT, "desktop-manager"); //perhaps query for interfaces?
    if(result != 0){
        puts("Failed to connect to \"desktop-manager\"");
        return 2;
    }

    typedef union{
    
        struct{
            uint32_t sender_pid;
            uint8_t frame_type;
            uint8_t data_start;
        };
        uint8_t raw_frame[512];
   } pex_client_frame_t;


    // pex_client_frame_t frame2send;
    // frame2send.frame_type = 0xc; //like text?
    // memcpy(&frame2send.data_start, "Here's some text darling\n", 26);
    // // write(pexfd, &frame2send, sizeof(pex_client_frame_t));

    // printf("And btw my pid is %u, as client process\n", getpid());
    pex_client_frame_t data;
    data.frame_type = 0xc;
    memcpy(&data.data_start, "Leave it in the lap of the gods\n", 33);
    puts("Let's actually write to it? from it\n");
    char buf;
    write(pexfd, &data, sizeof(data));
    // while( read(pexfd, &buf, 1) != 0){
    //     putchar(buf);
    // }

    return 0;
    // this will be forked from the window manager since i don't have sockets
    // socket fd's will be given through pipe and they are going to be in arguments
    // argv[0] = program name
    // argv[1] =  client read 
    // argv[2] = client write
    //let's try to write  aclient for our window manager

    if(argc != 3){
        printf("Usage test fd0 fd1\n");
        return 1;
    }

    int server_read =  atoi(argv[1]);
    int server_write = atoi(argv[2]);

    mds_create_window(server_write);
    mds_set_window_size(server_write, 200, 200);
    mds_create_string(server_write, 0, 0, "liar");
    mds_create_string(server_write, 0, 1 , "i have");
    // mds_create_string(server_write, 0, 2, "sailed");
    // mds_create_string(server_write, 0, 3, "the seas");
    
    
    
    while(1){
        int ch;
        
        if(read(server_read, &ch, 1) != -1){
            char line_buffer[42];
            sprintf(line_buffer, "received character %c:%x\r\n", ch, ch);
            write(2, line_buffer, strlen(line_buffer));
        }



    }
    
    return 0;
}
