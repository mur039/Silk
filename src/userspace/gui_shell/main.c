
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "glyph.h"
#include <sys/socket.h>


int puts(const char *str){
    return write(0, str, strlen(str));
}

void* memcpy4(void* dst, void* src, size_t dword_count ){

    uint32_t *_dst = dst, *_src = src;
    for(size_t i = 0 ; i < dword_count; ++i){
        _dst[i] = _src[i];
    }
    return dst;
}

void* memset4(void* dst, uint32_t value, size_t dword_count ){

    uint32_t *_dst = dst;
    for(size_t i = 0 ; i < dword_count; ++i){
        _dst[i] = value;
    }
    return dst;
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

            // char str[] = "a window";
            // for(int j  = 0; j < strlen(str); ++j){
                
            //     glyph_put_in(windows[i].bitmap, windows[i].width , str[j], 8*j, 0, bg, fg);
            // }



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

typedef struct{

    uint32_t* fb_base;
    uint32_t  fb_height;
    uint32_t  fb_width;

} framebuffer_t;


void draw_rect_on_fb(framebuffer_t fb, uint32_t x, uint32_t y, uint32_t width, uint32_t height, pixel_t colour){

    x =  x % fb.fb_width;
    y =  y % fb.fb_height;


    if(width > (x + fb.fb_width)){
        width = fb.fb_width - x;
        
    }

    if(height > (y + fb.fb_height)){
        height = fb.fb_height - y;
        
    }

    for(int i = 0; i < height; ++i){ //by row

        pixel_t* pixelptr = (pixel_t*)fb.fb_base;
        pixelptr += (i + y)*fb.fb_width;


        for(int j = 0; j < width; ++j){ //by col
            pixelptr[x + j] = colour;
        } 
    }


}


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
    printf("request id %u:%s\n", request_type, client_request_str[request_type]);
    switch (request_type){

    case CLIENT_CREATE_WINDOW:
        windows[0].exist = 1;
        windows[0].bg_color = (pixel_t){.red = 0x69, .green = 0x69, .blue = 0x69};
        windows[0].height = 200;
        windows[0].width = 200;
        windows[0].x = 0;
        windows[0].y = 0;
        windows[0].bitmap = NULL;
        change = 1;
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


void wait_mf(){
    for(int i = 0; i < 0xFFFFF; ++i){
        asm volatile("nop\n\t");
    }
}

#define GRPH_RED (pixel_t){.red = 0xFF, .green = 0x00, .blue = 0x00}
#define GRPH_GREEN (pixel_t){.red = 0x00, .green = 0xFF, .blue = 0x00}
#define GRPH_BLUE (pixel_t){.red = 0x00, .green = 0x00, .blue = 0xFF}

int main( int argc, char* argv[]){

    // i will fork and paint one side to ue and other to red
    malloc_init();

    //closing stdin would release it from parent shell?
    //bad but it is what it is

    close(FILENO_STDIN);
    int display_fd = open("/dev/fb0", O_WRONLY);    
    if(display_fd == -1){
        puts("failed to open /dev/fb0, exitting...");
        return 1;
    }

    uint16_t screenproportions[2];
    ioctl(display_fd, 0, screenproportions);

    frame_buffer_width = screenproportions[0];
    frame_buffer_height = screenproportions[1];
    printf("framebuffer width:height -> %u:%u\n", frame_buffer_width, frame_buffer_height);

    framebuffer_addr = mmap(NULL, frame_buffer_width*frame_buffer_height*4, 0, 0, display_fd, 0); //mapping framebuffer
    if(framebuffer_addr == (void*)-1){
        puts("failed to map framebuffer");
        framebuffer_addr = NULL;
        return 1;
    }



    //hopefully it's allocated
    backbuffer_addr = mmap(NULL, frame_buffer_width*frame_buffer_height*4 , 0, MAP_ANONYMOUS, -1, 0);
    if(backbuffer_addr == (void*)-1){
        
        puts("failed to allocate memory for backbuffer exiting");
        return 1;
    }

    printf("framebuffer:%x\n", framebuffer_addr);
    printf("backbuffer:%x\n", backbuffer_addr);

    framebuffer_t bf = {.fb_base = backbuffer_addr, .fb_height = frame_buffer_height, .fb_width = frame_buffer_width};


    //now let's create the server here
    int err;
    int servfd = socket(AF_UNIX, SOCK_RAW, 0);
    if(servfd < 0){
        puts("Failed to bind a socket :(");
        return 1;
    }

    struct sockaddr_un nix;    
    nix.sun_family = AF_UNIX;
    sprintf(nix.sun_path, "mds");

    err = bind(servfd, (struct sockaddr*)&nix, sizeof(nix));
    if(err < 0){
        puts("Failed to bind addr\n");
        return 1;
    }

    //i can only have one client at a time
    err = listen(servfd, 1);
    if(err < 0){
        puts("Error on the listen\n");
        return 1;
    }

    err = fcntl(servfd, F_GETFL, 0);
    fcntl(servfd, F_SETFL, err | O_NONBLOCK);


    int clientfd = -1;

    int x = 0, y = 0;
    while(1){
        
        //get a new client unblocking
        err = accept(servfd, NULL, NULL);
        if(clientfd == -1 && err > 0){
            puts("new client connected\n");
            clientfd = err; //and make it nonblocking as well
            err = fcntl(clientfd, F_GETFL, 0);
            fcntl(clientfd, F_SETFL, err | O_NONBLOCK);
        }

        
        memset4(backbuffer_addr, 0, frame_buffer_height * frame_buffer_width);

        if(clientfd < 0){ //we don''t have a client
            memcpy4(framebuffer_addr, backbuffer_addr, frame_buffer_height*frame_buffer_width);    
            continue;
        }

        //we have an client then
        int ch;        
        err = read(clientfd, &ch, 1);
        if(err == 0){
            change = 1;
            clientfd = -1;
            continue;
        }
        else if(err == -EAGAIN){
            continue;
        }
     
        if(ch == SERV_HEADER){ //incoming data
                
            struct ds_packet package;
            uint8_t * phead = &package;
            phead++;

            for(int i = 0; i < 2; ++i){
                read(clientfd, phead, 1);
                phead++;
            }

            int type = package.type;
            int length = package.length;

            for(int i = 0; i < (length - 3); ++i){
                read(clientfd, phead, 1);
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

        if(change){
            memset4(backbuffer_addr, 0, frame_buffer_height * frame_buffer_width);
            change = 0;

        }

        if (windows[0].exist)
        {
            window_t *win = &windows[0];
            draw_rect_on_fb(bf, win->x, win->y, win->width, win->height, win->bg_color);
        }

        memcpy4(framebuffer_addr, backbuffer_addr, frame_buffer_height*frame_buffer_width);    

    }


        
    return 0;
}
