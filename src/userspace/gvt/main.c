
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

#define MAX_NUMBER_OF_WINDOWS 1
int change = 0;


int main( int argc, char* argv[]){

    
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
    }

    //allocating new memory for backbuffer
    //hopefully it's gonna be linear
    backbuffer_addr = mmap(NULL, 0, 0, MAP_ANONYMOUS, -1, 0);
    for(int i = 1; i < 1 + (frame_buffer_width*frame_buffer_height*4 / 0x1000) ; ++i){
        mmap(NULL, 0, 0, MAP_ANONYMOUS, -1, 0);
    }


    printf("framebuffer:%x\n", framebuffer_addr);
    printf("backbuffer:%x\n", backbuffer_addr);

    //clearing the screen or painting it to black
    for(int i = 0; i < frame_buffer_height * frame_buffer_width; ++i){
        ((uint32_t*)backbuffer_addr)[i] = 0;
    }
    

    if (glyph_load_font("/share/screenfonts/consolefont.psf") == -1){
        
        puts("Failed to parse consolefont.psf\n");
    }    

    struct vt {
        uint32_t row, col;
        uint32_t curx, cury;
    } root;

    root.col = frame_buffer_width / 8;
    root.row = frame_buffer_width / get_glyph_size();
    root.curx = 0;
    root.cury = 0;

    pixel_t bg = {.green = 0};
    pixel_t fg = {.green = 0xff};
    int sfd = open("/dev/ttyS0", O_RDONLY);
    if(sfd < 0){
        puts("failed to open /dev/ttyS0\n");
        return 1;
    }

    //we are going to read from the serial port and place it 
    write(display_fd, backbuffer_addr, frame_buffer_height * frame_buffer_width * 4); //clear it

    char c;
    while( read(sfd, &c, 1) > 0){
        
        switch(c){
            
            case 0 ... 7:  //don't do a thing
            case 27: //for escape sequences
                break;
            case '\r':
            root.curx = 0;
            break;
            
            case '\n': 
                root.curx = 0;
                root.cury += 1;
            break;

            case '\b':
                if(root.curx){
                    root.curx -= 1;
                    glyph_put_in(backbuffer_addr, frame_buffer_width, ' ', root.curx * 8, root.cury * get_glyph_size(), bg, fg);

                }
            break;

            case '\t':
                root.curx += 4;
                break;
            case ' ':
                glyph_put_in(backbuffer_addr, frame_buffer_width, ' ', root.curx * 8, root.cury * get_glyph_size(), bg, fg);
                root.curx += 1;
                break;

            default:
                glyph_put_in(backbuffer_addr, frame_buffer_width, c, root.curx * 8, root.cury * get_glyph_size(), bg, fg);
                root.curx += 1;
                break;
        }

        if(root.curx >= root.col){
            root.curx  = 0;
            root.cury += 1;
        }

        if(root.cury >= root.row){
            
            root.curx = 0;
            root.cury -= 1;
            uint8_t* screen = backbuffer_addr;
            memcpy(screen, &screen[4*get_glyph_size()*frame_buffer_width], 4*frame_buffer_width*(frame_buffer_height));
        }

        write(display_fd, backbuffer_addr, frame_buffer_height * frame_buffer_width * 4); //write the changes

    }
    
    return 0;
}
