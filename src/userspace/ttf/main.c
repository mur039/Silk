#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "bitmap.h"
#include <silk/kd.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H




void* memset4(void* dst, unsigned long value, size_t dword_count ){

    unsigned long *_dst = dst;
    for(size_t i = 0 ; i < dword_count; ++i){
        _dst[i] = value;
    }
    return dst;
}


struct termios original;



#define ABGR2ARGB(colour) ( (colour & 0xff00ff00) | ((colour >> 2*8) & 0xff ) | ( (colour & 0xff) << 2*8 ) )

int main(int argc, char* argv[]){
    

    int display_fd = open("/dev/fb0", O_WRONLY);    
    if(display_fd == -1){
        puts("failed to open /dev/fb0, exitting...");
        return 1;
    }

    uint16_t screenproportions[2];
    ioctl(display_fd, 0, screenproportions);



    struct surface display_surface = {
        .s_width = screenproportions[0], 
        .s_height = screenproportions[1],
        .s_pitch =  screenproportions[0] * 4,
        .s_bpp = 32
    };

    u_int fb_size = display_surface.s_width * display_surface.s_height * (display_surface.s_bpp / 8);
    void* addr = mmap(NULL, fb_size, 0, 0, display_fd, 0); //mapping framebuffer
    if(addr == (void*)-1){
        puts("failed to map framebuffer");
        return 1;
    }

    display_surface.s_back = addr;
    display_surface.s_flags = SURFACE_ARGB;



    //freetype shit
    FT_Library  library;
    int err = FT_Init_FreeType(&library);
    if(err){
        printf("FUCKKK\n");
        return 1;
    }

    FT_Face     face;      /* handle to face object */
    err = FT_New_Face( library,
                     "/share/screenfonts/dejavusans.ttf",
                     0,
                     &face );
    if(err){
        printf("fucked font?\n");
        return 1;
    }

    FT_Set_Pixel_Sizes(face, 12, 24); // width=8px, height=16px per char
    int cell_w = face->max_advance_width >> 6;       // width of one cell
    int cell_h = (face->size->metrics.height >> 6);  // total line spacing

    err = FT_Load_Char(face, 'A', FT_LOAD_RENDER); // load and render glyph into bitmap
    FT_GlyphSlot g = face->glyph;
    cell_w += g->advance.x >> 6 ;


    printf("-> %u : %u\n",cell_w, cell_h); 


    struct fbcon con = {
        .fb_surface = &display_surface,
        .row = (display_surface.s_height / cell_h),
        .col = (display_surface.s_width / cell_w)
    };
   
    printf("row: %u col: %u\n", con.row, con.col);
    
    // return 0;
    ioctl(STDOUT_FILENO,KDSETMODE, KD_GRAPHICS); //set to kernel graphics
    
    //raw mode
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    fbcon_clear(&con);


    int pos_x = 0;    
    int pos_y = 0;    
    int c;
    int should_run = 1;
    while(should_run){

        c = getchar();
        
        switch(c){
            
            case 0:
            case 3:
                should_run = 0;
                continue;
                break;
            
                
                
                case '\n':
                con.pos_r++;
                break;
                
            case '\b':
                if(con.pos_c == 0) continue;
                con.pos_c--;
                c = ' ';
                
            default:

                err = FT_Load_Char(face, c, FT_LOAD_RENDER); // load and render glyph into bitmap
                if(err){
                    FT_Load_Char(face, '?', FT_LOAD_RENDER);
                }
                FT_GlyphSlot g = face->glyph;
                unsigned char *bitmap = g->bitmap.buffer;
                int w = g->bitmap.width;
                int h = g->bitmap.rows; 
            
                struct surface glyph_s = {.s_back = bitmap, .s_bpp = 1, .s_height = h, .s_width = w, .s_pitch = w};
            
                surface_copy_surface(&display_surface, &glyph_s, 
                    pos_x + g->bitmap_left, pos_y + cell_h - g->bitmap_top);      
                con.pos_c++;
            break;
        }
        
        
        if(con.pos_c >= con.col){
            con.pos_c = 0;
            con.pos_r++;
        }

        if(con.pos_r >= con.row){  //scrolling?
            con.pos_r = 0; //for now
        }

        pos_x = (con.pos_c * cell_w);
        pos_y = (con.pos_r * cell_h);
    }
    
    putchar('\n');    


}
