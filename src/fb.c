#include <fb.h>

static uint8_t *framebuffer_addr = NULL;
static uint32_t framebuffer_width;
static uint32_t framebuffer_heigth;


void init_framebuffer(void * address, int width, int height){
    framebuffer_addr = address;
    framebuffer_width = width;
    framebuffer_heigth = height;
    return;
}

void framebuffer_put_block(int width, int height){
    int x = 0;
    int y = 0;

    for (int j = 0; j < height; j++)
    {
        pixel_t * row = (pixel_t *)&framebuffer_addr[(y + j) * 4 * framebuffer_width];
        for(int i = 0; i < width; ++i){
             row[x + i].green = 0xff;
        }
    }
    
    
    return;
}


void framebuffer_put_glyph(const char symbol, int x, int y, pixel_t bg, pixel_t fg){
    if(framebuffer_addr == 0 || framebuffer_width == 0 || framebuffer_heigth == 0) return; //framebuffer is not initiliazed

    // uart_print(COM1, "framebuffer_put_glyph -> %c %x\r\n", symbol, symbol);

    uint8_t * glyph = get_glyph_bitmap(symbol);

     for (size_t j = 0; j < get_glyph_size(); j++)
    {
        pixel_t * row = (pixel_t *)&framebuffer_addr[(y + j) * 4 * framebuffer_width];
        for(int i = 0; i < 8; ++i){ //bits
            row[x + i]= GET_BIT(glyph[j], (7 -i)) ? fg : bg;
                
        }
    }
    

}

pixel_t fb_bg;
pixel_t fb_fg;
static int fb_cursor_x;
static int fb_cursor_y;
static int fb_console_cols;
static int fb_console_rows;

/*-1 means maximum*/
void init_fb_console(int cols, int rows){
    
    fb_console_cols =  (cols == -1) ? (int)(framebuffer_width / 8) :cols; 
    fb_console_rows =  (rows == -1) ? (int)(framebuffer_heigth / get_glyph_size()) :rows;
    fb_cursor_x = 0;
    fb_cursor_y = 0;

    fb_bg.red = 0x00;
    fb_bg.green = 0x00;
    fb_bg.blue = 0x00;

    fb_fg.red   = 0xff;
    fb_fg.green = 0xff;
    fb_fg.blue  = 0xff;
}

static int escape_sequence = 0;
void fb_console_putchar(char c){

    if(escape_sequence){
        if(c !='['){ //checking if its really escape sequence
            escape_sequence = 0; 
            return;
            }
 
    }

    switch (c)
    {
        case 27: //for escape sequences
            /*27 '[' 'H' -> move to 0,0*/ 
            escape_sequence = 1;
            break;
        
        case '\r':
        case '\n': 
            fb_cursor_x = 0;
            fb_cursor_y += 1;
            break;

        case '\b':
            if(fb_cursor_x){
                fb_cursor_x -= 1;
                framebuffer_put_glyph(' ', fb_cursor_x * 8, fb_cursor_y * get_glyph_size(), fb_bg, fb_fg);

            }
            break;
        case ' ':
            fb_cursor_x += 1;
            break;

        default:
            framebuffer_put_glyph(c, fb_cursor_x * 8, fb_cursor_y * get_glyph_size(), fb_bg, fb_fg );
            fb_cursor_x += 1;
            break;
    }
    

    if(fb_cursor_x >= fb_console_cols){ 
        fb_cursor_x = 0;
        fb_cursor_y += 1;
    }
    
    /*scrolling*/
    if(fb_cursor_y >= fb_console_rows){ 

        fb_cursor_x = 0; //already zero
        //some sort of memcpy here
        memcpy(framebuffer_addr, &framebuffer_addr[4*get_glyph_size()*framebuffer_width], 4*framebuffer_width*(framebuffer_heigth));
        fb_cursor_y -= 1;
    }

}

void fb_console_write(void * src, uint32_t size, uint32_t nmemb){
    uint8_t * source = src;
    for (size_t i = 0; i < size*nmemb; i++)
    {
        fb_console_putchar(source[i]);
    }
    
}

void fb_console_put(char *s){
    while(*s != '\0'){
        fb_console_putchar(*s);
        s++;
    }
}
