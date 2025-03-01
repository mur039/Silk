
#include <stdint-gcc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "glyph.h"


int puts(const char *str){
    return write(0, str, strlen(str));
}


typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;


uint32_t frame_buffer_width = 0;
uint32_t frame_buffer_height = 0;


pixel_t * framebuffer_addr = NULL;
pixel_t * backbuffer_addr = NULL;
typedef struct{
    uint32_t x, y;
    uint32_t width, height;
    pixel_t bg_color;
    int exist;
    pixel_t * bitmap;
} window_t;

#define MAX_NUMBER_OF_WINDOWS 1
int change = 0;
window_t windows[MAX_NUMBER_OF_WINDOWS];
window_t cursor;

void redraw_buffer(int fd){

    pixel_t pix;
    pix.red = 0;
    pix.green = 0;
    pix.blue = 0;


    // //clear everything
    memset(backbuffer_addr, 0, frame_buffer_width*frame_buffer_height*4);

    for(int i = 0; i  < MAX_NUMBER_OF_WINDOWS; ++i){

        if(windows[i].exist){
            int x,y;
            x = windows[i].x;
            y = windows[i].y;

            if(x + windows[i].width > frame_buffer_width){
                x = frame_buffer_width - windows[i].width;
            }

            if(y + windows[i].height > frame_buffer_height){
                y = frame_buffer_height - windows[i].height;
            }

            pixel_t pixel = windows[i].bg_color;

            for(int h = 0 ; h < windows[i].height; ++h){

                uint32_t offset =  ((y + h)* frame_buffer_width) + x ;
                for(int w = 0 ; w < windows[i].width; ++w){
                    
                    backbuffer_addr[offset + w] = windows[i].bitmap[ (h * windows[i].width)  + w];
                    
                }
            }

            
            // pixel_t bg = {.red = 0x00, .green = 0x00, .blue = 0x00};
            // pixel_t fg = {.red = 0x00, .green = 0xff, .blue = 0xff};

            // // char str[] = "a window";
            // // for(int j  = 0; j < strlen(str); ++j){
                
            // //     glyph_put_in(windows[i].bitmap, windows[i].width , str[j], 8*j, 0, bg, fg);
            // // }



        }
    }
    


    if(cursor.exist){
            int x,y;
            x = cursor.x;
            y = cursor.y;

            if(x + cursor.width > frame_buffer_width){
                x = frame_buffer_width - cursor.width;
            }

            if(y + cursor.height > frame_buffer_height){
                y = frame_buffer_height - cursor.height;
            }

            pixel_t pixel = cursor.bg_color;

            for(int h = 0 ; h < cursor.height; ++h){

                uint32_t offset =  ((y + h)* frame_buffer_width) + x ;
                // lseek(fd , ( (y + h) * 4 * frame_buffer_width) + x*4 ,SEEK_SET);

                for(int w = 0 ; w < cursor.width; ++w){
                    
                    backbuffer_addr[offset + w] = pixel;
                    // write(fd ,&pixel, sizeof(pixel_t));    
                    
                }
            }

        }

    if(framebuffer_addr){

    memcpy(framebuffer_addr, backbuffer_addr, frame_buffer_width*frame_buffer_height*4);
    }
    else{
        lseek(fd, 0, SEEK_SET);
        write(fd, backbuffer_addr, frame_buffer_width*frame_buffer_height*4);
    }

    change = 0;

}


typedef union 
{
    struct 
    {
        union status{
            struct
            {
                char y0:1;
                char x0:1;
                char ys:1;
                char xs:1;
                char one:1;
                char bm:1;
                char br:1;
                char bl:1;    
            };
            char raw;
        } bits;
        signed char x_axis_move;
        signed char y_axis_move;
    };
    unsigned int raw; //struct itself is only 3-byte
} ps2_mouse_generic_package_t;


enum server_bytecodes{
    SERV_REQUEST = 0,
    SERV_REPLY,
    SERV_EVENT,
    SERV_HEADER = 0x69 //nice

};

const char* server_bytecodes_str[] = {

    [SERV_REQUEST] = "SERV_REQUEST",
    [SERV_REPLY] = "SERV_REPLY",
    [SERV_EVENT] = "SERV_EVENT",


    [SERV_HEADER] = "SERV_HEADER",
};


enum server_requesnt{
    CLIENT_CREATE_WINDOW = 1,
    CLIENT_SET_WINDOW_SIZE,
    CLIENT_DESTROY_WINDOW,
    CLIENT_ASCII_STRING


};

const char* client_request_str[] = {

    [CLIENT_CREATE_WINDOW] = "CLIENT_CREATE_WINDOW",
    [CLIENT_SET_WINDOW_SIZE] = "CLIENT_SET_WINDOW_SIZE",
    [CLIENT_DESTROY_WINDOW] = "CLIENT_DESTROY_WINDOW",
    [CLIENT_ASCII_STRING] = "CLIENT_ASCII_STRING"



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

int process_request(struct ds_packet package){
    
    uint8_t request_type = package.opt0;
    printf("%s\n", client_request_str[request_type]);
    switch (request_type){

    case CLIENT_CREATE_WINDOW:
        windows[0].exist = 1;
        windows[0].bg_color = (pixel_t){.red = 0x69, .green = 0x69, .blue = 0x69};
        windows[0].x = 0;
        windows[0].y = 0;
        windows[0].bitmap = NULL;

        break;
    
    case CLIENT_SET_WINDOW_SIZE:
        ;
        int width = package.opt1, height = package.opt2;
        printf(":%ux%u\n", width, height);
        windows[0].width = width;
        windows[0].height = height;
        change = 1;
        
        int pagesize = (4 * width * height / 4096) + ( (4 * width * height) % 4096 != 0);
        for(int i = 0; i < pagesize; ++i ){

            
            void * ptr = mmap(0, 0, 0, MAP_ANONYMOUS, -1, 0);
            memset(ptr, 0, 4096);
            printf("mmapd %u: %p\n",i,  ptr);

            if(i == 0){
                windows[0].bitmap = ptr;
            }
        
        }
        
        pixel_t bg = {.red = 0x69, .green = 0x69, .blue = 0x69};
        printf("windows[0].bitmap : %p\n", windows[0].bitmap);
        for(int i = 0; i < (width * height); ++i){
            windows[0].bitmap[i] = bg;
        
        }
        
        
        break;
    
    case CLIENT_DESTROY_WINDOW:
        windows[0].exist = 0;
        change = 1;
        break;
    
    case CLIENT_ASCII_STRING:
        ;
        int x = package.opt1;
        int y = package.opt2;
        

        //max 10 characters
        char * str = (char *)&package.opt3;
        for(int i = 0; i < strlen(str) && i < 10; ++i){
            glyph_put_in(
                windows[0].bitmap, 
                windows[0].width, 
                str[i], 
                8 * i, 
                y*get_glyph_size(), 
                windows[0].bg_color,
                (pixel_t){.red = 0xff, .green = 0xff, .blue = 0xff}
                );
        }
        break;


    default:
        printf("unknown request type %x\n", request_type);
        return -1;
        break;
    }

}


int main( int argc, char* argv[]){

    //asuumption screen is 800*600*4 size
    // i will fork and paint one side to ue and other to red
    malloc_init();
    
    int display_fd = open("/dev/fb0", O_WRONLY);    
    if(display_fd == -1){
        puts("failed to open /dev/fb0, exitting...");
        return 1;
    }

    uint16_t screenproportions[2];
    ioctl(display_fd, 0, screenproportions);

    frame_buffer_width = screenproportions[0];
    frame_buffer_height = screenproportions[1];

    framebuffer_addr = mmap(NULL, frame_buffer_width*frame_buffer_height*4, 0, 0, display_fd, 0); //mapping framebuffer
    if(framebuffer_addr == (void*)-1){
        puts("failed to map display_fd, will use  write function");
        framebuffer_addr = NULL;
        // return 1;
    }

    //hopefully it's gonna be linear
    backbuffer_addr = mmap(NULL, 0, 0, MAP_ANONYMOUS, -1, 0);
    for(int i = 1; i < 1 + (frame_buffer_width*frame_buffer_height*4 / 4096) ; ++i){
        mmap(NULL, 0, 0, MAP_ANONYMOUS, -1, 0);
    }


    printf("framebuffer:%x\n", framebuffer_addr);
    printf("backbuffer:%x\n", backbuffer_addr);

    for(int i = 0; i < frame_buffer_height * frame_buffer_width; ++i){
        ((uint32_t*)backbuffer_addr)[i] = 0x00FFFFFF;
    }
    
    write(display_fd, backbuffer_addr, frame_buffer_height * frame_buffer_width * 4);
    return 0;
    while(1){
    }


    // int mouse_fd = open("/dev/mouse", O_RDONLY);
    // int kbd_fd = open("/dev/kbd", O_RDONLY);

    int kbd_fd = -1;
    int mouse_fd = -1;
    if (glyph_load_font("/share/screenfonts/consolefont.psf") == -1){
        
        puts("Failed to parse consolefont.psf\n");
    }    
    
    change = 1;


    //well before that lets bind the manager here
    int pexfd = open("/dev/pex0", O_RDWR);
    if(pexfd < 0){
        puts("Failed to open /dev/pex0\n");
        return 1;
    }

    int result;
    result = ioctl(pexfd, 1, "desktop-manager"); //bind
    if(result != 0){
        puts("binding \"desktop-manager\" failed\n");
        return 1;
    }

    puts("binded \"desktop-manager\" service.\n"); //hopefully
    

    

    /*
    fuck that shit here how it goes, server process binds, client process connect
    it write to file, client side is responsible of converting raw data to data frames
    and send write them to file desriptor. At kernel side, by looking at the fields,
    adds some field like send time, who sent it etc
    */

   typedef union{
    
    struct{
        uint32_t sender_pid;
        uint8_t frame_type;
        uint8_t data_start;
    };
    
    uint8_t raw_frame[512];
   } pex_server_frame_t;

    
    cursor.exist = 1;
    cursor.height = 10;
    cursor.width = 10;
    cursor.bg_color.blue = 0xff;
    cursor.bg_color.red = 0xff;
    cursor.bg_color.green = 0xff;

    int x = 0;
    int y = 0;

    for(int i = 0; i < MAX_NUMBER_OF_WINDOWS; ++i)
        windows[i].exist = 0;

    int client_sock[2]; // [0]=r [1]=w
    if(pipe(client_sock) == -1){
        puts("failed to create pipe.");
        return 1;
    }

    int pid = fork();

    if(pid == -1){
        puts("Failed to fork, exiting...");
        return 1;
    }

    if(pid == 0){ //child process   
        uint32_t args[4];
        const char ** _argv = (const char**)&args;
        const char * executable_path = "/bin/test";
        
        printf("pipe r/w: %u %u\n", client_sock[0],client_sock[1]);
        
        char fd_0[2] = " ";
        char fd_1[2] = " ";
        sprintf(fd_0, "%u", client_sock[0]);
        sprintf(fd_1, "%u", client_sock[1]);

        args[0] = (uint32_t)executable_path;
        args[1] = (uint32_t)fd_0;
        args[2] = (uint32_t)fd_1;
        args[3] = (uint32_t)0;

        int result = execve(
                            executable_path,
                            _argv
                            );

        if(result == -1){
            puts("execve failed.");
            return 1;
        }

    }

    
    change = 1;
    while(1){
        
        // pex_server_frame_t frame;
        // if(read(pexfd, &frame, sizeof(frame))){

        //     printf("frame sender pid: %u, frame_type:%x\n", frame.sender_pid, frame.frame_type);

        //     if(frame.frame_type == 0xc){

        //         puts(&frame.data_start);
        //     }
        // }
        // else{
        //     return 0;
        // }

    }


    while (1){
        
        int ch;        
        if( read(client_sock[0], &ch, 1) != -1){
            
            if(ch == SERV_HEADER){ //incoming data
                
                struct ds_packet package;
                uint8_t * phead = &package;
                phead++;

                for(int i = 0; i < 2; ++i){
                    read(client_sock[0], phead, 1);
                    phead++;
                }

                int type = package.type;
                int length = package.length;

                for(int i = 0; i < (length - 3); ++i){
                    read(client_sock[0], phead, 1);
                    phead++;
                }

                switch (type){

                case SERV_REQUEST:
                    printf(
                        "received request %s length:%u\n", 
                        server_bytecodes_str[type],
                        length
                        );

                    process_request(package);

                    break;
                
                case SERV_REPLY:
                    puts("do we need SERV_REPLY in the server?\n");
                    break;
                
                case SERV_EVENT:
                    puts("do we need SERV_EVENT in the server?\n");
                    break;

                default:
                    printf("unknown packet type %x\n", type);
                    break;
                }

            }

            

        
        }
    
    }
    



        
    return 0;
}
