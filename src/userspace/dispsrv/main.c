#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <termios.h>
#include <silk/kd.h>
#include <sys/mman.h>
#include <poll.h>
#include <sys/socket.h>
#include <math.h>

#define OLIVEC_IMPLEMENTATION
#include "olive.c"


#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"




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

#define MAX_CLIENTS 2
int display_server(){

    int err;
    int display_fd = open("/dev/fb0", O_WRONLY);    
    if(display_fd == -1){
        puts("failed to open /dev/fb0, exitting...");
        return 1;
    }

    //try to open cursor image
    int x, y, noc;
    stbi_uc* cursorimage = stbi_load("/share/wallpaper.jpg", &x, &y, &noc, 0);
    if(!cursorimage){
        printf("Failed to load cursor image. because: %s\n", stbi_failure_reason());
        return 1;   
    }

    printf("x:%u y:%u noc:%u\n", x, y, noc);

    uint16_t screenproportions[2];
    ioctl(display_fd, 0, screenproportions);

    size_t screen_width = screenproportions[0];
    size_t screen_heigth = screenproportions[1];

    display0.bpp = 4;
    display0.width = screen_width;
    display0.height = screen_heigth;
    display0.pitch = display0.width * display0.bpp;

    u_int fb_size = display0.width * display0.height * display0.bpp;
    display0.fb_addr = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, display_fd, 0); //mapping framebuffer
    if(display0.fb_addr == (void*)-1){
        puts("failed to map framebuffer");
        return 1;
    }
    
    unsigned char* d = cursorimage;
    unsigned long* fb = (void*)display0.fb_addr;
    
    for(int j = 0; j < y; ++j){
        for(int i = 0; i < x; ++i){
            
            unsigned long pix = 0;
            for(int k = 0; k < noc; ++k){
                pix |= *(d++) << (8*k);
            }
            fb[i + j*display0.width] = pix;
        }    
    }

    
    for(unsigned long x = 0; x < screen_width; x++){

        //sin (= [-1, 1]
        //fuck that schietze
        // 0,0      0,1
        //
        //1,0       1,1
        //sin  + 1, (0, 2)
        double rad = 2 * M_PI * 5 * ((double)x/screen_width);
        double normalized = ((sin(rad)*0.9 + 1)/2);
        unsigned long* row = &fb[   (int)(screen_heigth * normalized) * screen_width ];
        row[x] = 0xff00ff;
    }
    
    // memcpy4(display0.fb_addr, cursorimage, x * y * noc);


    return 0;









    void* backbuffer = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if(backbuffer == (void*)-1){
        puts("failed to allocate memory for backbuffer\n");
        return 1;
    }

    display1 = display0;
    display1.fb_addr = backbuffer;
    

    // ioctl(STDOUT_FILENO,KDSETMODE, KD_GRAPHICS);
    char c;
    
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);

    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 0; //practically non-blocking
    t.c_cc[VTIME] = 0;//practically non-blocking 
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    int psauxfd = open("/dev/psaux", O_RDONLY);
    int acceptfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    sprintf(addr.sun_path, "dispsrv");
    err = bind(acceptfd, (struct sockaddr*)&addr, sizeof(addr));
    if(err < 0){
        perror("bind");
        return 1;
    }
    listen(acceptfd, MAX_CLIENTS);

    //okay we are polling 'em john
    // psaux, tty are the input we expect, and 
    //for the clients, we will poll accep socket
    //as well as the connected ones
    
    struct pollfd polls[3 + MAX_CLIENTS];
    struct pollfd* clientpoll = &polls[3];
    size_t pollables = 3;

    memset(polls, 0, sizeof(polls));
    
    //stdin
    polls[0].fd = STDIN_FILENO;
    polls[0].events = POLLIN | POLLOUT;

    //psaux
    polls[1].fd = psauxfd;
    polls[1].events = POLLIN;

    //acceptfd
    polls[2].fd = acceptfd;
    polls[2].events = POLLIN;
    
    //main loop
    while(1){
        err = poll(polls, pollables, 16); // 16ms
        if(err <= 0) continue;
        
        //check for the inputs and accept queueus;
        for(int i = 0; i < 3; ++i){
            if(polls[i].revents & (POLLIN | POLLOUT)){

                if(polls[i].fd == STDIN_FILENO){
                    int c;
                    read(STDIN_FILENO, &c, 1);

                    if(c == 'q'){
                        exit(0);
                    }else{
                        printf("[tty]: keyinput %c\n", c);
                    }
                }
                else if(polls[i].fd == psauxfd){
                    int psaux  = 0;
                    read(psauxfd, &psaux, 3);
                    printf("[psaux]: read package %x\n", psaux);
                }
                else if(polls[i].fd == acceptfd){
                
                    int connfd = accept(acceptfd, NULL, NULL );
                    printf("[accept]: new connection %u, send little message, closed it\n", connfd);
                    write(connfd, "Hello little darling", 21);
                    close(connfd);
                    // polls[pollables].fd = connfd;
                    // polls[pollables].events = POLLIN | POLLOUT;
                    // polls[pollables].revents = 0;
                    // pollables++;
                }

                polls[i].revents = 0;   
            }
        }


        //now check the clients
        for(int i = 3; i < pollables; i++){

        }
        
    }
    
    return 0;
}







//if main program crashes, this will restore terminal to well first of all text mode and some sane defaults
int main(){
    atexit(cleanup);
    tcgetattr(STDIN_FILENO, &original);

    if(fork() == 0){
        return display_server();
    }


    while(1){

        pid_t err = wait4(-1, NULL, 0, NULL);    
        if( err < 0){ //some error
            
            if( errno == ECHILD){ //no child
                break;
            }
        }
    }    
    return 0;
}
