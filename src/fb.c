#include <fb.h>

static uint8_t *framebuffer_addr = NULL;
static uint32_t framebuffer_width;
static uint32_t framebuffer_heigth;


uint8_t * get_framebuffer_address(void){
    return framebuffer_addr;
}

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

uint8_t framebuffer_write_wrapper(uint8_t * buffer, uint32_t offset, uint32_t len, void * dev){
    dev;
    framebuffer_raw_write(offset, buffer, len);
    return 0;
}


int framebuffer_raw_write(size_t start, void * src, size_t count){
    memcpy(&framebuffer_addr[start], src, count);
    return 0;
}


void framebuffer_put_glyph(const unsigned short symbol, int x, int y, pixel_t bg, pixel_t fg){
    if(framebuffer_addr == 0 || framebuffer_width == 0 || framebuffer_heigth == 0) return; //framebuffer is not initiliazed

    // uart_print(COM1, "framebuffer_put_glyph -> %c %x\r\n", symbol, symbol);

    uint8_t * glyph = get_glyph_bitmap(symbol);

     for (size_t j = 0; j < get_glyph_size(); j++)
    {
        pixel_t * row = (pixel_t *)&framebuffer_addr[(y + j) * 4 * framebuffer_width];
        for(int i = 0; i < 8; ++i){ //bits
            row[x + i] = 
                        GET_BIT(glyph[j], (7 -i)) ? fg : bg;
                
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

void fb_set_console_color(pixel_t fg, pixel_t bg){
    fb_fg = fg;
    fb_bg = bg;
    return;
}

static int escape_sequence = 0;
static int number = 0;
void fb_console_putchar(char c){

    if(escape_sequence == 1){
        if(c =='['){ //checking if its really escape sequence
            escape_sequence = 2;
            return;
            }
 
    }
    if(escape_sequence == 2){ //we are in escape sequence but which one?
        switch (c)
        {
        case '0'...'1': //fuck it we go ESC[XY, x number, Y command
            number = c - '0';
            break;

        case 'A': 
            fb_cursor_y -= 1;
            if(fb_cursor_y <= 0) fb_cursor_y = 0;
            
            break;

        case 'B':
            fb_cursor_y++;
            escape_sequence = 0;
            return;
            break;

        case 'C':
            fb_cursor_x -= number;
            escape_sequence = 0;
            return;
            break;
            
        case 'D':
            fb_cursor_x -= number;
            escape_sequence = 0;
            return;
            break;
        case 'H': //move cursor to (0, 0)
            fb_cursor_x = 0;
            fb_cursor_y = 0;
            escape_sequence = 0;
            return;
        
        case 'J': //erase from cursor to the end of screen;
            escape_sequence = 0;
            int x,y;
            x = fb_cursor_x;
            y = fb_cursor_y;

            int total_characters2write = (fb_console_rows - y - 1) * fb_console_cols;
            total_characters2write += fb_console_cols - x;
            for(int i = 0; i < total_characters2write; ++i){
                fb_console_putchar(' ');
            }

            fb_cursor_x = x;
            fb_cursor_y = y;
            return;
        
        default:
            break;
        }
    }


    
if(escape_sequence == 0){
    switch (c)
    {
        // case '\0': 
        //     break;
        case 0:  //don't do a thing
            break;
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
        case '\t':
            fb_cursor_x += 4;
            break;
        case ' ':
            framebuffer_put_glyph(' ', fb_cursor_x * 8, fb_cursor_y * get_glyph_size(), fb_bg, fb_fg );

            fb_cursor_x += 1;
            break;

        default:
            framebuffer_put_glyph(c, fb_cursor_x * 8, fb_cursor_y * get_glyph_size(), fb_bg, fb_fg );
            fb_cursor_x += 1;
            break;
    }
    
}

    if(fb_cursor_x >= fb_console_cols){ 
        fb_cursor_x = 0;
        fb_cursor_y += 1;
    }
    
    /*scrolling*/
    if(fb_cursor_y >= fb_console_rows){ 

        fb_cursor_x = 0; //already zero
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

void fb_console_blink_cursor(){
    //block blinking

     for (size_t j = 0; j < get_glyph_size(); j++)
    {
        pixel_t * row = (pixel_t *)&framebuffer_addr[(fb_cursor_y*get_glyph_size() + j) * 4 * framebuffer_width];
        for(int i = 0; i < 8; ++i){ //bits
            if(
                row[fb_cursor_x*8 + i].alpha == fb_fg.alpha &&
                row[fb_cursor_x*8 + i].red == fb_fg.red     &&
                row[fb_cursor_x*8 + i].green == fb_fg.green &&
                row[fb_cursor_x*8 + i].blue == fb_fg.blue
                
            ){
                memcpy( &row[fb_cursor_x*8 + i], &fb_bg, sizeof(pixel_t));
            }
            else{
                memcpy( &row[fb_cursor_x*8 + i], &fb_fg, sizeof(pixel_t));
            }
            
        }
    }
    

}


static char fb_console_gprintf_wrapper(char c){
    fb_console_putchar(c);
    return c;
}

void fb_console_printf(const char * fmt, ...){
    va_list arg;
    va_start(arg, fmt);
    va_gprintf(
        fb_console_gprintf_wrapper,
        fmt,
        arg
    );
    return;
}