#include <fb.h>
#include <timer.h>
#include <process.h>

static int framebuffer_type;
static uint8_t *framebuffer_addr = NULL;
uint32_t framebuffer_width;
uint32_t framebuffer_heigth;


uint8_t * get_framebuffer_address(void){
    return framebuffer_addr;
}

//1 RGB, 2 text mode
void init_framebuffer(void * address, int width, int height, int type){
    
    framebuffer_addr = FRAMEBUFFER_VADDR;
    framebuffer_width = width;
    framebuffer_heigth = height;
    framebuffer_type = type;

    //now base on that check whether specified pages mapped or not
    uint8_t* start_addr = address;
    size_t nof_pages = width*(height + 1);
    nof_pages *= (type == 1 ? 4 : 2);
    nof_pages = (nof_pages/4096) + ((nof_pages % 4096) != 0);
    
    uint8_t* vaddr = (uint8_t*)FRAMEBUFFER_VADDR;
    for(size_t i = 0; i <= nof_pages; ++i){
        map_virtaddr(vaddr, start_addr, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_WRITE_THROUGH);
        start_addr += 4096;
        vaddr += 4096;
    }


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
    uint32_t dword_section = count / 4;
    uint32_t byte_section = count % 4;
    
    uint32_t* screen = (uint32_t*)framebuffer_addr;
    uint32_t* dwordptr = src;
    //fill out the byte section
    
    for(size_t i = 0; i < dword_section; ++i){
        screen[i] = dwordptr[i];
    }


    for(size_t i = 0; i < byte_section; ++i){
        uint8_t* bytescreen = (uint8_t*)&screen[dword_section];
        uint8_t* bytesptr = (uint8_t*)&dwordptr[dword_section];

        bytescreen[i] = bytesptr[i];
    }

    // memcpy(&framebuffer_addr[start], src, count);
    return count;
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

pixel_t term_8_16_colors[10] ={
    {.red = 0x00, .green = 0x00, .blue = 0x00}, //black
    {.red = 0xff, .green = 0x00, .blue = 0x00}, //red
    {.red = 0x00, .green = 0xff, .blue = 0x00}, //green
    {.red = 0xff, .green = 0xff, .blue = 0x00}, //yellow
    {.red = 0x00, .green = 0x00, .blue = 0xff}, //blue
    {.red = 0xff, .green = 0x00, .blue = 0xff}, //magenta
    {.red = 0x00, .green = 0xff, .blue = 0xff}, //cyan
    {.red = 0xff, .green = 0xff, .blue = 0xff}, //white
    {.red = 0x00, .green = 0x00, .blue = 0x00}, //default
    {.red = 0x00, .green = 0x00, .blue = 0x00} //like null?
};


pixel_t fb_bg;
pixel_t fb_fg;
static int fb_cursor_x;
static int fb_cursor_y;
static int fb_console_cols;
static int fb_console_rows;
//cursor shit
int fb_cursor_state = 0;
static uint8_t *under_cursor_area = NULL;

void cursor_blinker(void* some){

    

}
/*-1 means maximum*/

uint32_t init_fb_console(int cols, int rows){
    
    if(framebuffer_type == 1){

        fb_console_cols =  (cols == -1) ? (int)(framebuffer_width / 8) :cols; 
        fb_console_rows =  (rows == -1) ? (int)(framebuffer_heigth / get_glyph_size()) :rows;
        // timer_register(500, 500, cursor_blinker, NULL);
    }
    else{
        fb_console_cols = framebuffer_width;
        fb_console_rows = framebuffer_heigth;
    }

    fb_cursor_x = 0;
    fb_cursor_y = 0;

    fb_bg.red = 0x00;
    fb_bg.green = 0x00;
    fb_bg.blue = 0x00;

    fb_fg.red   = 0xff;
    fb_fg.green = 0xff;
    fb_fg.blue  = 0xff;

    uint32_t retval;
    retval = fb_console_rows & 0xffff;
    retval |= (fb_console_cols & 0xffff) << 16; 

    return retval;   
}

void fb_set_console_color(pixel_t fg, pixel_t bg){
    fb_fg = fg;
    fb_bg = bg;
    return;
}


typedef enum{
    TERM_ESCAPE_TOKEN_ALPHANUMERIC,
    TERM_ESCAPE_TOKEN_SEPERATOR,
    TERM_ESCAPE_TOKEN_END
} term_escape_token_type_t;

typedef struct{
    term_escape_token_type_t type;
    char value[8];
} term_escape_token_t;

typedef struct{
    const char* input;
    size_t position;
} term_escape_tokenizer_t;


static  int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static  int is_special(char c) {
    return c == ';' ;
}

static void term_escape_next_token(term_escape_tokenizer_t* tokenizer, term_escape_token_t* token){
    
    // Skip whitespace
    while (is_whitespace(tokenizer->input[tokenizer->position])) {
        tokenizer->position++;
    }

    char c = tokenizer->input[tokenizer->position];

    //end of input
    if(c == '\0'){
        token->type = TERM_ESCAPE_TOKEN_END;
        token->value[0] = '\0';
        return;
    }

    if(is_special(c)){ //pass over it i don't have any use for it
        tokenizer->position++;
    }

    //handle the rest darling
    size_t i = 0;
    while (tokenizer->input[tokenizer->position] && !is_whitespace(tokenizer->input[tokenizer->position]) &&
           !is_special(tokenizer->input[tokenizer->position])) {
        if (i < 16 - 1) {
            token->value[i++] = tokenizer->input[tokenizer->position];
        }
        tokenizer->position++;
    }
    token->value[i] = '\0';
    token->type = TERM_ESCAPE_TOKEN_ALPHANUMERIC;


}

static size_t term_escape_code_tokenize(char* input, term_escape_token_t* tokens, size_t max_tokens){
    
    term_escape_tokenizer_t tokenizer = {.input = input, .position = 0};
    size_t token_count = 0;

    while (token_count < max_tokens) {
        term_escape_token_t token;
        term_escape_next_token(&tokenizer, &token);
        tokens[token_count++] = token;
        if (token.type == TERM_ESCAPE_TOKEN_END) break;
    }

    return token_count - 1; // Exclude TOKEN_END from count
}


#include <g_list.h>
static int escape_sequence = 0;
#define TTY_CONSOLE_ESCAPE_ENCODER_BUFFER_SIZE 32
static int tty_console_escape_encoder_buffer_head = 0;
static char tty_console_escape_encoder_buffer[TTY_CONSOLE_ESCAPE_ENCODER_BUFFER_SIZE]; 
uint8_t utf8_buffer[4];
size_t expected_bytes = 0;
size_t received_bytes = 0;

void fb_console_putchar(unsigned short c){

#ifdef UTF8
    //utf shenanigans
    if(!expected_bytes){

        if(c >= 0x80){
    
            if((c & 0xE0) == 0xC0){
                expected_bytes = 2;
                received_bytes = 1;
                utf8_buffer[0] = c;
                return;
            }
            else if ((c & 0xF0) == 0xE0)
                { // three  byte
                expected_bytes = 3;
                received_bytes = 1;
                utf8_buffer[0] = c;
                return;
            }
            else if ((c & 0xF8) == 0xF0)
            { // four  byte
                expected_bytes = 4;
                received_bytes = 1;
                utf8_buffer[0] = c;
                return;
            }
            
            //we might try to print unicodes so
            goto normal_printing;
            
        }
        //do nothing for 7-bit characters they just go through

    }

    uint32_t val = 0;
    if(expected_bytes){
        if(c & 0xc0 == 0x80){ //continuation byte happy
            utf8_buffer[received_bytes++] = c;
            if(received_bytes >= expected_bytes){
                uint8_t* ptr = utf8_buffer;
                switch(expected_bytes){
                    case 2:

                        val = (*ptr & 0x1F) << 6;
                        val |= (*(++ptr) & 0x3F);
                        break;
                    case 3:

                        val = (*ptr & 0xF) << 12;
                        val |= (*(++ptr) & 0x3F) << 6;
                        val |= (*(++ptr) & 0x3F);
                        break;
                    case 4:

                        val = (*ptr & 0x7) << 18;
                        val |= (*(++ptr) & 0x3F) << 12;
                        val |= (*(++ptr) & 0x3F) << 6;
                        val |= (*(++ptr) & 0x3F);
                        break;
                }

                expected_bytes = 0;
                received_bytes = 0;
                goto normal_printing;
            }
        }
        else{ //my streak is broken :/
            expected_bytes = 0;
            received_bytes = 0;
        }
    }
#endif

    //before that if cursor is on the screen we disable it, since every write goes through this function
    if(fb_cursor_state ) cursor_blinker(NULL);


    /*for all escape sequences all of them ends with a alphabetical character
    most of the time values between [ and last alphabetical character is a argument in
     terms of numerical caharacters.there are some commands that [ {character} {argument} {opcode?}
     but i will pass them for now
    */
    if(escape_sequence){
        
        if(!tty_console_escape_encoder_buffer_head && c != '['){ //first characer after /x1b is [
            escape_sequence = 0; //just print the character then
            goto normal_printing; //forgive me father for that i sinned. nuh uh
        }


        tty_console_escape_encoder_buffer[tty_console_escape_encoder_buffer_head++] = c;
        tty_console_escape_encoder_buffer[tty_console_escape_encoder_buffer_head] = '\0';

        //check if current buffer is valid?
        // /x1b [ {numerical_string(s)} {alphabetical character}
        //parse it?
        if(IS_ALPHABETICAL(c)){ //supposedly on the end
            escape_sequence = 0; 

            tty_console_escape_encoder_buffer[--tty_console_escape_encoder_buffer_head] = '\0';

            term_escape_token_t tokens[17];
            size_t token_count = term_escape_code_tokenize(&tty_console_escape_encoder_buffer[1], tokens, 16);

            int opcode = c;
        
            //maybe convert that list into an array of pointers?
            char** escape_code_args = kcalloc(token_count + 1, sizeof(char*));
            char** head = escape_code_args;

            for(size_t i = 0; i < token_count; ++i){
                *(head++) = tokens[i].value;
            }


            //opcodes from : https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
            switch (opcode) {

            case 'H': //move cursor to \x1b[{line};{col}H of \x1b[H
                if(token_count){
                    if(token_count != 2){
                        break;
                    }

                    int line, col;
                    line = atoi(escape_code_args[0]) - 1;
                    col = atoi(escape_code_args[1]) - 1;


                    // fb_console_printf("ESC[%u;%uH\n", line, col);

                    line = line < 0 ? 0 : line > fb_console_rows ? fb_console_rows : line;
                    col = col < 0 ? 0 : col > fb_console_cols ? fb_console_cols : col;

                    fb_cursor_x = col;
                    fb_cursor_y = line;
                }
                else{ //size == 0   
                    fb_cursor_x = 0;
                    fb_cursor_y = 0;
                }
                break;

            case 'f': //move cursor to \x1b[{line};{col}f
                if(token_count == 2){

                    int line, col;
                    line = atoi(escape_code_args[0]);
                    col = atoi(escape_code_args[1]);

                    line = line < 0 ? 0 : line > fb_console_rows ? fb_console_rows : line;
                    col = col < 0 ? 0 : col > fb_console_cols ? fb_console_cols : col;

                    fb_cursor_x = col;
                    fb_cursor_y = line;
                }
                break;            

            case 'A': //move cursor to \x1b[{line}A
            case 'B':
            case 'E':
            case 'F':

                if(token_count == 1){
                    int number_of_lines = atoi(escape_code_args[0]);
                    int line = fb_cursor_y;

                    line +=  (opcode == 'A' || opcode == 'F' ?-1 : 1) * number_of_lines;
                    line = line < 0 ? 0 : line > fb_console_rows ? fb_console_rows : line;

                    fb_cursor_y = line;

                    if( opcode == 'E' || opcode == 'F'){
                        fb_cursor_x = 0;
                    }

                }
                break;
            

            case 'C': //move cursor to \x1b[{column}C
            case 'D':
                if(token_count == 1){
                    int number_of_cols = atoi(escape_code_args[0]);
                    int col = fb_cursor_x;
                    col +=  (opcode == 'C' ? 1 :-1) * number_of_cols;

                    col = col < 0 ? 0 : col > fb_console_cols ? fb_console_cols : col;
                    fb_cursor_x = col;
                }
                break;


            //erase functions
            case 'J':
            case 'K':
                if(token_count){
                    if(token_count == 1){
                        int selector = atoi(escape_code_args[0]);

                        switch(selector){
                            case 0:
                                for(int y = fb_cursor_y; y < (opcode == 'J' ? fb_console_rows : fb_cursor_y + 1) ; ++y){
                                   for(int x = y == fb_cursor_y ? fb_cursor_x : 0; x < fb_console_cols; ++x){
                                        framebuffer_put_glyph(' ', x * 8, y * get_glyph_size(), fb_bg, fb_fg);
                                    }
                                }
                                break;
                            case 1:
                                break;
                            case 2:
                                //clear all the screen;
                                if(opcode != 'J') break;
                                for(int y = 0; y < fb_console_rows ; ++y){
                                    for(int x = 0 ; x < fb_console_cols; ++x){
                                         framebuffer_put_glyph(' ', x * 8, y * get_glyph_size(), fb_bg, fb_fg);
                                     }
                                 }
                                break;

                            default:break;

                        }


                    }
                }
                else{ // [J [K
                    if(opcode == 'J'){ //deleter from cursor to end of screen
                        
                        for(int y = fb_cursor_y; y < fb_console_rows; ++y){
                            for(int x = y == fb_cursor_y ? fb_cursor_x : 0; x < fb_console_cols; ++x){
                                framebuffer_put_glyph(' ', x * 8, y * get_glyph_size(), fb_bg, fb_fg);
                            }
                        }
                    }
                    else{ //K erase in line
                        for(int i = 0; i < fb_console_cols; ++i){
                            framebuffer_put_glyph(' ', i * 8, fb_cursor_y * get_glyph_size(), fb_bg, fb_fg );
                        }   
                    }
                }
                break;


            case 'm': //shapes and colours
                
                if(token_count && token_count <= 5 ){ //max 3 args min 1 argument
                    
                    int mode, foreground, background;
                    
                    {
                        int color_args[5] = {-1, -1, -1, -1, -1}; //atoi(escape_code_args[0]); //must exist
                        for(unsigned int i = 0; i < token_count; ++i){
                            color_args[i] = atoi(escape_code_args[i]); //must exist
                        }

                        mode = color_args[0];
                        foreground = color_args[1];
                        background = color_args[2];
                    
                    }
                    
                    switch(mode){
                        case 0: //clear the mode 
                            fb_bg.red = 0x00;
                            fb_bg.green = 0x00;
                            fb_bg.blue = 0x00;

                            fb_fg.red   = 0xff;
                            fb_fg.green = 0xff;
                            fb_fg.blue  = 0xff;
                            break;


                        case 38: //set foreground 256 color
                        case 48: //set background 256 color
                            if(foreground == 2){ //set colour as rgb
                                
                                int red = atoi(escape_code_args[2])
                                 ,green = atoi(escape_code_args[3])
                                  ,blue = atoi(escape_code_args[4]) ;

                                pixel_t * pix = &fb_bg;

                                if(mode == 38){
                                    pix = &fb_fg;
                                }

                                pix->red = red;
                                pix->green = green;
                                pix->blue = blue;

                            }
                            break;

                        default: //don't care about the mode set background and foreground colors accordingly
                            if(foreground != -1){
                                int colour_index =  foreground - 30; //0 1 , 2 ,3; for reset = -30
                                if( colour_index < 0) colour_index = 9;
                                if(colour_index == 9) colour_index = 7;

                                fb_fg = term_8_16_colors[colour_index];
                            }

                            if(background != -1){
                                int colour_index =  background - 40; //0 1 , 2 ,3; for reset = -30
                                if( colour_index < 0) colour_index = 9;
                                if(colour_index == 9) colour_index = 0;

                                fb_bg = term_8_16_colors[colour_index];
                            }

                            break;

                    }


                }
                break;

            default:
                
                break;
            }


            kfree(escape_code_args);
            escape_code_args = NULL;
            
            tty_console_escape_encoder_buffer_head = 0;
            memset(tty_console_escape_encoder_buffer, 0, TTY_CONSOLE_ESCAPE_ENCODER_BUFFER_SIZE);
        }   


        return;
    }
    

normal_printing:
    switch (c)
    {

        case 0:  //don't do a thing
            break;
        case 27: //for escape sequences
            /*27 '[' 'H' -> move to 0,0*/ 
            escape_sequence = 1;
            break;
        
        case '\r':
            fb_cursor_x = 0;
            break;
            
        case '\n': 
            fb_cursor_x = 0;
            fb_cursor_y += 1;
            break;

        case 127:
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
    


    if(fb_cursor_x >= fb_console_cols){ 
        fb_cursor_x = 0;
        fb_cursor_y += 1;
    }
    
    /*scrolling*/
    if(fb_cursor_y >= fb_console_rows){ 

        fb_cursor_x = 0; //already zero

        uint8_t *fb_start, *fb_end;
        fb_start = framebuffer_addr;
        fb_end = fb_start + framebuffer_width * 4 * framebuffer_heigth;

        fb_start += framebuffer_width * 4 * get_glyph_size();
        
        memcpyw4(framebuffer_addr, fb_start, (size_t)(fb_end - fb_start)/4);

        fb_start = fb_end - framebuffer_width * 4 * get_glyph_size();
        
        memsetw4(fb_start, *(uint32_t*)&fb_bg, (size_t)(fb_end - fb_start) / 4);

        // uint32_t *_begin = (uint32_t*)framebuffer_addr, *_begin2 = (uint32_t*)&framebuffer_addr[4*get_glyph_size()*framebuffer_width];
        // for(size_t i = 0; i < framebuffer_width*framebuffer_heigth; ++i){
        //     _begin[i]  = _begin2[i];
        // }
        // // memcpy(framebuffer_addr, &framebuffer_addr[4*get_glyph_size()*framebuffer_width], 4*framebuffer_width*(framebuffer_heigth));

        // for(int x = 0; x < fb_console_cols; ++x)
        //     framebuffer_put_glyph(' ', x * 8, fb_cursor_y * get_glyph_size(), fb_bg, fb_fg );

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


uint32_t fb_write(fs_node_t * node, uint32_t offset, uint32_t size, uint8_t* buffer){
    (void)node;
    return framebuffer_raw_write(offset, buffer, size);
}   



static int framebuffer_id = 0;

typedef struct{
    uint32_t* baseaddr;
    uint16_t  width;
    uint16_t  height;
    uint16_t  bpp;
    uint16_t  misc;
} framebuffer_t;



static int framebuffer_ioctl(fs_node_t* node, unsigned long request, void* argp){

    device_t* dev = node->device;
    framebuffer_t* fb = dev->priv;
    uint16_t *lengths = (uint16_t*)argp;

    switch(request){
        case 0:
            lengths[0] = fb->width;
            lengths[1] = fb->height;
            break;

        case 1:
            ;
            uint32_t* addr = argp;
            *addr = (uint32_t)fb->baseaddr;
            break;
        default:
            return 1;
    }
    
    return 0;
}

void framebuffer_close(fs_node_t* node){
    
    return;
}

void* framebuffer_mmap(fs_node_t* node, size_t length, int prot, size_t offset){

    device_t* dev = node->device;
    framebuffer_t* fb = dev->priv;

    //for now offset is ignored and length must match the framebuffer
    if(length != fb->height*fb->width*(fb->bpp/8u) || offset != 0){
        return (void*)-1;
    }

    size_t roundedup_length = ((length/4096) + (length % 4096 != 0)) * 4096;
    void* candidate_addr = vmm_get_empty_vpage(current_process->mem_mapping, roundedup_length);

    if(candidate_addr == (void*)-1)  
        return (void*)-1;

    for(size_t i = 0; i < roundedup_length ; i += 4096){
        map_virtaddr( (uint8_t*)candidate_addr + i, (uint8_t*)fb->baseaddr + i, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_WRITE_THROUGH | PAGE_USER_SUPERVISOR );
        vmm_mark_allocated(current_process->mem_mapping, (uint32_t)candidate_addr + i, (uint32_t)fb->baseaddr + i, VMM_ATTR_PHYSICAL_MMIO);
    }
    
    return candidate_addr;
}

void install_basic_framebuffer(uint32_t* base, uint32_t width, uint32_t height, uint32_t bpp){
    device_t framebuffer;
    memset(&framebuffer, 0, sizeof(framebuffer));
    framebuffer.name = strdup("fbxxx");
    sprintf(framebuffer.name, "fb%u", framebuffer_id++);

    framebuffer.write = (write_type_t)fb_write;
    framebuffer.ioctl = (ioctl_type_t)framebuffer_ioctl;
    framebuffer.close = (close_type_t)framebuffer_close;
    framebuffer.mmap = (mmap_type_t)framebuffer_mmap;
    framebuffer.dev_type = DEVICE_CHAR;
    framebuffer.unique_id = 29;

    framebuffer_t* priv = kcalloc(1, sizeof(framebuffer_t));
    priv->baseaddr = base;
    priv->width = width;
    priv->height = height;
    priv->bpp = bpp;

    framebuffer.priv = priv;
    dev_register(&framebuffer);
}