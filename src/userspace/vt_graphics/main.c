#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <termios.h>
#include <silk/kd.h>

#define OLIVEC_IMPLEMENTATION
#include "olive.c"



void* memcpy4(void* dst, void* src, size_t dword_count ){

    unsigned long *_dst = dst, *_src = src;
    for(size_t i = 0 ; i < dword_count; ++i){
        _dst[i] = _src[i];
    }
    return dst;
}

void* memset4(void* dst, unsigned long value, size_t dword_count ){

    unsigned long *_dst = dst;
    for(size_t i = 0 ; i < dword_count; ++i){
        _dst[i] = value;
    }
    return dst;
}

struct disp{
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;
    uint32_t* fb_addr;
};

struct disp display0;
struct disp display1;
struct termios original;


void cleanup(){
    ioctl(STDOUT_FILENO, KDSETMODE, KD_TEXT);
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
}    



#define ABGR2ARGB(colour) ( (colour & 0xff00ff00) | ((colour >> 2*8) & 0xff ) | ( (colour & 0xff) << 2*8 ) )


#define BACKGROUND_COLOR 0xFF202020
#define FOREGROUND_COLOR 0xFF2020FF



int main(){
    
    atexit(cleanup);

    int display_fd = open("/dev/fb0", O_WRONLY);    
    if(display_fd == -1){
        puts("failed to open /dev/fb0, exitting...");
        return 1;
    }

    uint16_t screenproportions[2];
    ioctl(display_fd, 0, screenproportions);

    display0.bpp = 4;
    display0.width = screenproportions[0];
    display0.height = screenproportions[1];
    display0.pitch = display0.width * display0.bpp;

    u_int fb_size = display0.width * display0.height * display0.bpp;
    display0.fb_addr = mmap(NULL, fb_size, 0, 0, display_fd, 0); //mapping framebuffer
    if(display0.fb_addr == (void*)-1){
        puts("failed to map framebuffer");
        return 1;
    }

    void* backbuffer = mmap(NULL, fb_size, 0, MAP_ANONYMOUS, -1, 0);
    if(backbuffer == (void*)-1){
        puts("failed to allocate memory for backbuffer\n");
        return 1;
    }

    display1 = display0;
    display1.fb_addr = backbuffer;
    

    ioctl(STDOUT_FILENO,KDSETMODE, KD_GRAPHICS);
    printf("i set it to graphics mode\n" );
    char c;
    
    fcntl( FILENO_STDIN, F_SETFL ,fcntl(FILENO_STDIN, F_GETFL) | O_NONBLOCK);
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    original = t;

    t.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    
    Olivec_Canvas oc = olivec_canvas(display0.fb_addr, display0.width, display0.height, display0.width);
    Olivec_Canvas b_buff = olivec_canvas(display1.fb_addr, display1.width, display1.height, display1.width);
    
    
    while(1){
        //screen must be cleared darling
        for(int i = 0; i < oc.width - 80; i += 10){
            
            if(read(FILENO_STDIN, &c, 1)){
                if(c == 'q') return 0;
            }

            //write to backbuffer
            olivec_fill(b_buff, ABGR2ARGB(OLIVEC_RGBA(0xff, 0xff, 0xff, 0xff)));
            olivec_rect(b_buff, i, oc.height/2, 80, 80, ABGR2ARGB(OLIVEC_RGBA(0xff, 0, 0, 0xff)));

            //maybe i should have created drm manager :/
            //copy backbuffer to the main buffer
            memcpy4(oc.pixels, b_buff.pixels, oc.height*oc.width);

        }
    }
    
        
    

    
    


}
