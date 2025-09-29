#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdint.h>



enum tftp_opcode{
    TFTP_RRQ = 1,
    TFTP_WRQ = 2,
    TFTP_DATA = 3,
    TFTP_ACK = 4,
    TFTP_ERROR = 5,
};

enum tftp_error_code{
    NOT_DEFINED= 0,
    FILE_NOT_FOUND = 1,
    ACCESS_VIOLATION = 2,
    DISK_FULL = 3,
    ILLEGAL_OP = 4,
    UNKNOWN_TID = 5,
    FILE_ALREADY_EXIST = 6,
    NO_SUCH_USER = 7,
};

int tftp_reply_error(int sockfd, const struct sockaddr* addr, socklen_t len, int error_code, const char*  err_msg){

    int err;
    size_t packlen = 4 + strlen(err_msg) + 1;
    char* buffer = malloc(packlen);
    
    char* head = buffer;

    *(head++) = '\0';
    *(head++) = TFTP_ERROR;
    *(head++) = '\0';
    *(head++) = error_code;
    sprintf(head, err_msg);

 
    err = sendto(sockfd, buffer, packlen, 0, addr, len);
    sleep(1);
    free(buffer);
    return err;
}



int daemon_loop(){
    int err;
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t socklen = sizeof(server_addr);

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(69);
    server_addr.sin_addr = 0; //INANYADDR

    err = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err < 0){
        return 1;
    }


    unsigned char buffer[512];

    while(1){
        err = recvfrom(sockfd, &buffer, 512, 0, (struct sockaddr*)&server_addr, &socklen);
        
        if(err > 0){
            unsigned char* head = buffer;
            unsigned char* tail = buffer + err;


            unsigned short opcode = *head++;
            opcode = (opcode << 8) | *head++;


            switch(opcode){
                case TFTP_RRQ:{
                    
                    struct sockaddr_in client_addr = server_addr;
                    socklen_t client_size = socklen;
                    int replyfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket;

                    const char* filename = head;
                    const char* mode = filename + strlen(filename) + 1;
                    printf("[RRQ]filename:%s and mode:%s\n", filename, mode);

                    if(strcmp(mode, "octet")){
                        tftp_reply_error(replyfd, (struct sockaddr*)&client_addr, client_size, ILLEGAL_OP, "file mode not supported");
                        close(replyfd);
                        break;
                    }
                    
                    FILE* file = fopen(filename, "rb");
                    if(!file){

                        tftp_reply_error(replyfd, (struct sockaddr*)&client_addr, client_size, FILE_NOT_FOUND, "No such file");
                        close(replyfd);    
                        break;
                    }

                    //now we can do some shit
                    unsigned char lbuff[520];
                    for(int i = 1; i < 65536; ++i){
                        err = fread(buffer, 1, 512, file);
                        // err = read(file->_fd, buffer, 512);
                        lbuff[0] = 0;
                        lbuff[1] = TFTP_DATA;
                        lbuff[2] = (i >> 8) & 0xff;
                        lbuff[3] = (i >> 0) & 0xff;
                        memcpy(&lbuff[4], buffer, err);
                        sendto(replyfd, lbuff, 4 + err, 0, (struct sockaddr*)&client_addr, client_size);

                        if(err < 512){ //EOF
                            break;
                        }
                    }
                    
                    sleep(1);
                    fclose(file);
                    close(replyfd);
                }

                case TFTP_WRQ:{
                    const char* filename = head;
                    const char* mode = filename + strlen(filename) + 1;
                    printf("[WRQ]filename:%s and mode:%s\n", filename, mode);

                    struct sockaddr_in client_addr = server_addr;
                    socklen_t client_size = socklen;
                    int replyfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket;
                    tftp_reply_error(replyfd, (struct sockaddr*)&client_addr, client_size, ILLEGAL_OP, "Writes are not allowed");
                    close(replyfd);
                }
                break;
                default:break;
            }

        }

    }

    return 0;
}

int main(int argc, char* argv[]){

    int daemon = 0;
    for(int i = 1; i < argc; ++i){
        if(!strcmp(argv[i], "-d")){
            daemon = 1;
        }
    }

    if(daemon){
        return daemon_loop();
    }
    
    return 0;
}