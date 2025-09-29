#include <network/inet_socket.h>
#include <network/udp.h>
#include <network/tcp.h>
#include <syscalls.h>


struct socket* inet_socket_create(int type, int protocol){

    int err;
    struct socket* socket;
    switch(type){
        
        case SOCK_DGRAM:

            socket = socket_allocate_socket();
            if(!socket)
                return NULL;
            
            socket->type = type;
            socket->state = SS_UNCONNECTED;
            socket->flags = 0;
            
            err = udp_create_sock(socket);
            if(err < 0){
                socket->type = SS_FREE;
                return NULL;
            }
            
        break;


        case SOCK_STREAM:
            socket = socket_allocate_socket();
            if(!socket)
                return NULL;
            
            socket->type = type;
            socket->state = SS_UNCONNECTED;
            socket->flags = 0;
            
            err = tcp_create_socket(socket);
            if(err < 0){
                socket->type = SS_FREE;
                return NULL;
            }
        break;

        //well actually...
        case SOCK_RAW:
            fb_console_printf("ah, a SOCK_RAW\n");
            socket = NULL;
        break;
        default:
            fb_console_printf("ah, a i don't know what this is: %u\n", type);
            socket = NULL;
        break;

    }
    
    return socket;
}