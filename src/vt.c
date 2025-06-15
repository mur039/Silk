#include <vt.h>
#include <pmm.h>
#include <process.h>
#include <fb.h>

int vt_count;
struct vt *virtualterms = NULL;
struct vt *active_vt;



int vt_tty_send(int _scancode){

    char kb_character = _scancode & 0xFF;
    int scancode = (_scancode >> 8) & 0xFF;
    int modifier = (_scancode >> 16) & 0xFF;

    int ctrl, alt, shift;

    ctrl = (modifier >> 0) & 1;
    shift = (modifier >> 1) & 1;
    alt = (modifier >> 2) & 1;
    
    if(alt && ((scancode - 59) >= 0 && (scancode - 59) < 10) ){
    
        
        if(active_vt->vtmode.mode != VT_AUTO){
            //must ask the process by sending signal?
            return -1;
        }
        //vt switching darling
        int curr_index = (int)(active_vt - virtualterms);
        int next_index = (scancode - 59);

        active_vt = &virtualterms[next_index];

        if(curr_index != next_index){

            //flush the write buffer
            int ch;
            vt_redraw();
            while( (ch = circular_buffer_getc(&active_vt->tty.write_buffer)) != -1 ){
                active_vt->tty.driver->put_char(&active_vt->tty, ch);
            }
        }

        return 0;
    }


    struct tty* activetty = &active_vt->tty;

    if( 
        // !activetty->refcount || 
        !activetty->session 
    )
    {
        return 0;
    }

    return tty_ld_write(activetty, &kb_character, 1);
}



void tty_driver_vt_putchar(struct tty* tty, char c){
    
    int active_index = (int)(active_vt - virtualterms);
    
    if(active_index != tty->index){

        circular_buffer_putc(&tty->write_buffer, c);
        return;
    }
    
    vt_console_putchar(c);
    return;
}


void tty_driver_vt_write(struct tty* tty, const char* buf, size_t len){

    for(size_t i = 0;i < len; ++i){
        tty->driver->put_char(tty, buf[i]);
    }
}

struct tty_driver vt_driver = {
    
    .name = "vt",
    .num = 2,
    .write =  &tty_driver_vt_write,
    .put_char = &tty_driver_vt_putchar
};



void vt_csr_blinker(void* vs){
    
    int crsr_state = (active_vt->kmode >> 1) & 3;
    int crsr_en = crsr_state & 1;
    int crsr_visible = (crsr_state >> 1) & 1;
    //okay check if cursor is disabled and is it on screen
 
    if(crsr_en){

        char chr, attr;
        chr = active_vt->textbuff[ active_vt->tty.size.ws_col * active_vt->cursor_pos[1] + active_vt->cursor_pos[0]];
        attr = active_vt->attribute; //active_vt->attrbuff[ active_vt->tty.size.ws_col * active_vt->cursor_pos[1] + active_vt->cursor_pos[0]];
        extern pixel_t term_8_16_colors[10];
        pixel_t fb_bg, fb_fg;
        fb_bg = term_8_16_colors[ attr  &0xF];
        fb_fg = term_8_16_colors[ (attr & 0xF0) >> 4];
        crsr_visible ^= 1;

        if(crsr_visible){
            framebuffer_put_glyph(U'█', active_vt->cursor_pos[0] * 8, active_vt->cursor_pos[1] * get_glyph_size(), fb_bg, fb_fg );
        }
        else{
            framebuffer_put_glyph(chr, active_vt->cursor_pos[0] * 8, active_vt->cursor_pos[1] * get_glyph_size(), fb_bg, fb_fg );
        }

        //place it together
        active_vt->kmode &=  ~(3ul << 1);
        active_vt->kmode |=  ((crsr_visible << 1) | crsr_en) << 1;
    }
}


int install_virtual_terminals(int count, int row, int cols){
    
    virtualterms = kcalloc(count, sizeof(struct vt));
    if(!virtualterms){
        return -1;
    }   
    
    timer_register(500, 500, vt_csr_blinker, NULL);

    for(int i = 0; i < count ; ++i){
        device_t dev;
        dev.dev_type = DEVICE_CHAR;
        dev.name = strdup("ttyxxx");
        sprintf(dev.name, "tty%u", i);
        dev.unique_id = i;

        dev.open  = (open_type_t)tty_open;
        dev.close = (close_type_t)tty_close;
        dev.write = (write_type_t)tty_write;
        dev.read = (read_type_t)tty_read;
        dev.ioctl = (ioctl_type_t)tty_ioctl; //????

        struct vt* vt = &virtualterms[i];
        struct tty* tty = &vt->tty;
        dev.priv =  tty;

        vt->kmode = 0 | (1 << 1);
        vt->cursor_pos[0] = 0;
        vt->cursor_pos[1] = 0;
        vt->vtmode.mode = VT_AUTO;
        vt->vtmode.waitv = 0;
        vt->attribute = 0x70;
        vt->textbuff = kmalloc(cols*row);
        vt->attrbuff = kmalloc(cols*row);

        memset(vt->textbuff, ' ', cols*row);
        memset(vt->attrbuff, 0x70, cols*row);
        memset(vt->esc_encoder, 0, sizeof(vt->esc_encoder));

        tty->size.ws_col = cols;
        tty->size.ws_row = row;
        tty->index = i;
        tty->termio.c_lflag = ISIG | ICANON | ECHO;
        tty->read_wait_queue = list_create();
        tty->read_buffer = circular_buffer_create(4096);
        tty->write_buffer = circular_buffer_create(4096);
        tty->linebuffer = circular_buffer_create(512);
        tty->driver = &vt_driver;
        
        dev_register(&dev);
    }
    vt_count = count;
    active_vt = virtualterms;
    return count;
}

void vt_console_putchar(unsigned short c){

    if(active_vt->kmode & 0b100){ // if visible then
        vt_csr_blinker(NULL);
    }
    // if(fb_cursor_state ) cursor_blinker(NULL);
    //actually buffer for unicode as well?????
    if(active_vt->esc_encoder[0] == '\e'){ //non empty and possiblt valid? encoder

        if(strlen(active_vt->esc_encoder) == 1 && c != '['){
            memset(active_vt->esc_encoder, 0, sizeof(active_vt->esc_encoder));
            return;
        }

        
        //overflow so flush and return
        if(strlen(active_vt->esc_encoder) >= sizeof(active_vt->esc_encoder)){
            memset(active_vt->esc_encoder, 0, sizeof(active_vt->esc_encoder));
            return;
        }
        active_vt->esc_encoder[strlen(active_vt->esc_encoder)] = c;


        if(! IS_ALPHABETICAL(c)  ) { //possible argument or seperator //we just encoded them 
            return;
        }

        int opcode = c;
        char* buffhead = &active_vt->esc_encoder[0];
        char* bufftail = &active_vt->esc_encoder[strlen(active_vt->esc_encoder) - 1];

        //sanity check wouldn't hurt anybody
        if( !(buffhead[0] == '\e'&&  buffhead[1] == '[') ){
            memset(active_vt->esc_encoder, 0, sizeof(active_vt->esc_encoder));
            return;
        }
        buffhead += 2; //okay consume ^[[

        //some varibale that might be useful
        size_t lincursorpos = active_vt->cursor_pos[0] + (active_vt->cursor_pos[1]*active_vt->tty.size.ws_col) ;
        size_t totalsize = active_vt->tty.size.ws_row*active_vt->tty.size.ws_col;

        int state = 0; //misc
        int arguments[8] = {0, 0, 0, 0, 0, 0, 0, 0}; //max
        
        switch(opcode){


            case 'H':  //^[[H   ^[[{line};{col}H
                if(bufftail[-1] == '['){ //just ^[[H
                    active_vt->cursor_pos[0] = 0;
                    active_vt->cursor_pos[1] = 0;
                    break;
                }

            __attribute__((fallthrough));
            //same as the ^[[{line};{col}H
            case 'f': 
                // a state variable i use later 
                // like this fella takes 2 decimals as args, 
                // if one doesn't exist or more than 2 or non decimal arguments then fail
                state = 0; 
                while(buffhead < bufftail){
                   // ought to be decimal;decimal

                   if(state == 0){ //read until ;
                    if(*buffhead == ';'){
                        state = 1;
                        ++buffhead; //consume the ;
                        continue;
                    }
                    if( !IS_NUMBER(*buffhead)){
                        state = -1;
                        break;
                    }
                    arguments[0] *= 10;
                    arguments[0] += *buffhead - '0';
                    buffhead++;
                   }
                   else if(state == 1){ //read until the opcode
                    //hmm since we start to parse when a alphabatic character comes 
                    // it should be all numeric but some motherfucker might send non alphanumeric character
                    // or an extra argument so

                    if( !IS_NUMBER(*buffhead)){ //hmmmmmm
                        state = -1;
                        break;
                    }
                    arguments[1] *= 10;
                    arguments[1] += *buffhead - '0';
                    buffhead++;
                   }
                }

                if(state < 0) break; //invalid req

                active_vt->cursor_pos[0] = arguments[1] > 1 ? arguments[1] - 1 : 0;
                active_vt->cursor_pos[1] = arguments[0] > 1 ? arguments[0] - 1 : 0;
            break;
            
            
            case 'K':
                state = 1;
            __attribute__((fallthrough));
            case 'J': //^[[J == ^[[0J or K

                while ((buffhead < bufftail)){ //^[[{0, 1, 2, 3}J
                    if( !IS_NUMBER(*buffhead)){ //hmmmmmm
                        state = -1;
                        break;
                    }
                    arguments[0] *= 10;
                    arguments[0] += *buffhead - '0';
                    buffhead++;
                }

                if(state < 0) break;

                int op = arguments[0];
                if(op < 0 || op > 3) break;

                //defaulting to beginng and end of screen
                char* erase_start = active_vt->textbuff;
                char* erase_end = active_vt->textbuff + totalsize;



                if(op == 0){ //from cursor
                    
                    erase_start += lincursorpos; 
                    if(state){ //K //end of line
                        erase_end = &active_vt->textbuff[ active_vt->tty.size.ws_col * (active_vt->cursor_pos[1] + 1) ];
                    }
                    //erase end of screen
                }
                else if(op == 1){ //from beginning of * to cursor

                    if(state){ //K //beginning of line
                        erase_start = &active_vt->textbuff[ active_vt->tty.size.ws_col * active_vt->cursor_pos[1] ];
                    }
                    //from beginning of screen
                    
                    erase_end = &active_vt->textbuff[lincursorpos];

                }
                else if(op == 2){
                    //J erase entire screen
                    if(state){ //K  //entire line
                        erase_start = &active_vt->textbuff[ active_vt->tty.size.ws_col * active_vt->cursor_pos[1] ];
                        erase_end = &active_vt->textbuff[ active_vt->tty.size.ws_col * (active_vt->cursor_pos[1] + 1) ];
                    }
                                        
                }
                else if(op == 3){ //erase saved lines?
                    //nop
                    break;
                }
                memset(erase_start, ' ', (uint32_t)(erase_end - erase_start) ); //erase entire screen
                vt_redraw();
            break;
            
            case 'm': //shapes and colours
                ;
                int * argpts = arguments;
                while(buffhead < bufftail){

                    //hmm what about '[[; ???
                    if(*buffhead == ';'){
                        
                        state += 1;
                        argpts++;
                        ++buffhead; //consume the ;
                        continue;
                    }

                    if( !IS_NUMBER(*buffhead)){ //hmmmmmm
                        state = -1;
                        break;
                    }
                    
                    *argpts *= 10;
                    *argpts += *buffhead - '0';
                    buffhead++;
                }

                if(state < 0)
                    break;
                
                argpts = arguments;
                for(int i = 0; i <= state; ++i){
                    
                    if(argpts[i] == 0) active_vt->attribute = 0x70;

                    if( //foreground
                        (argpts[i] >= 30 && argpts[i] <= 37)
                        // || (argpts[i] >= 90 && argpts <= 97) //no bright colours
                    )
                    {
                        active_vt->attribute &= 0x0F;
                        active_vt->attribute |=  (argpts[i] - 30) << 4;
                    }

                    if( //background
                        (argpts[i] >= 40 && argpts[i] <= 47)
                        // || (argpts[i] >= 100 && argpts <= 107) //no bright colours
                    )
                    {
                        active_vt->attribute &= 0xF0;
                        active_vt->attribute |=  (argpts[i] - 40) << 0;
                    }

                }

            break;

            default:
            break;
        }


        memset(active_vt->esc_encoder, 0, sizeof(active_vt->esc_encoder));
        return;
    }


    extern pixel_t term_8_16_colors[10];
    pixel_t fb_bg, fb_fg;
    fb_bg = term_8_16_colors[ active_vt->attribute  &0xF];
    fb_fg = term_8_16_colors[ (active_vt->attribute & 0xF0) >> 4];;


    switch (c){

    case 0: // don't do a thing
    break;
    
    case '\e': // for escape sequences
        //is shouldn't be here when receiving esci if i'am then i shouldn't receive escape code during a escape code
        memset(active_vt->esc_encoder, 0, sizeof(active_vt->esc_encoder));
        active_vt->esc_encoder[0] = '\e';
    break;

    case '\r':
        active_vt->cursor_pos[0] = 0;
    break;

    case '\n':
        active_vt->cursor_pos[0] = 0;
        active_vt->cursor_pos[1] += 1;
    break;

    case 127:
    case '\b':
        if (active_vt->cursor_pos[0]){
            
            active_vt->cursor_pos[0]-= 1;
            framebuffer_put_glyph(' ', active_vt->cursor_pos[0] * 8, active_vt->cursor_pos[1] * get_glyph_size(), fb_bg, fb_fg);
            char* row = &active_vt->textbuff[ active_vt->tty.size.ws_col * active_vt->cursor_pos[1] ];
            row[active_vt->cursor_pos[0]] = ' ';
        }
    break;
    
    case '\t':
        active_vt->cursor_pos[0] += 4;
    break;
    
    default:
        framebuffer_put_glyph(c, active_vt->cursor_pos[0] * 8, active_vt->cursor_pos[1] * get_glyph_size(), fb_bg, fb_fg);
        char* row = &active_vt->textbuff[ active_vt->tty.size.ws_col * active_vt->cursor_pos[1] ];
        char* attrrow = &active_vt->attrbuff[ active_vt->tty.size.ws_col * active_vt->cursor_pos[1] ];
        
        row[active_vt->cursor_pos[0]] = c & 0xFF;
        attrrow[active_vt->cursor_pos[0]] = active_vt->attribute;
        active_vt->cursor_pos[0] += 1;
    break;
    }

    if (active_vt->cursor_pos[0] >= active_vt->tty.size.ws_col){
        active_vt->cursor_pos[0] = 0;
        active_vt->cursor_pos[1] += 1;
    }

    /*scrolling*/
    if (active_vt->cursor_pos[1] >= active_vt->tty.size.ws_row){


        active_vt->cursor_pos[0] = 0; // already zero
        active_vt->cursor_pos[1] -= 1;


        //scroll video memory
        extern uint32_t framebuffer_width, framebuffer_heigth;
        uint8_t* framebuffer_addr =  get_framebuffer_address();
        
        uint8_t *fb_start, *fb_end;
        fb_start = framebuffer_addr;
        fb_end = fb_start + framebuffer_width * 4 * framebuffer_heigth;

        fb_start += framebuffer_width * 4 * get_glyph_size();
        memcpyw4(framebuffer_addr, fb_start, (size_t)(fb_end - fb_start)/4);

        fb_start = fb_end - framebuffer_width * 4 * get_glyph_size();
        
        memsetw4(fb_start, *(uint32_t*)&fb_bg, (size_t)(fb_end - fb_start) / 4);
        



        //scrolling text-buffer
        char *text_start, *text_end, *attr_start, *attr_end;
        size_t totalsize = active_vt->tty.size.ws_row*active_vt->tty.size.ws_col;

        text_start = active_vt->textbuff;
        text_end =  text_start + totalsize;

        attr_start = active_vt->attrbuff;
        attr_end =  attr_start + totalsize;

        text_start += active_vt->tty.size.ws_col;
        attr_start += active_vt->tty.size.ws_col;

        memcpy(active_vt->textbuff, text_start, (size_t)(text_end - text_start) );

        text_start = text_end - active_vt->tty.size.ws_col;
        memset( text_start , ' ', (size_t)(text_end - text_start));

        memcpy(active_vt->attrbuff, attr_start, (size_t)(attr_end - attr_start));

        attr_start = attr_end - active_vt->tty.size.ws_col;
        memset( attr_start, active_vt->attribute, (size_t)(attr_end - attr_start));

    }

}


//draw text buffer to the screen
int vt_redraw(){
    char* textbuffer = active_vt->textbuff;
    char* attrbuffer = active_vt->attrbuff;
    pixel_t fg, bg;
    extern pixel_t term_8_16_colors[10];
    

    for(unsigned int i = 0; i  < active_vt->tty.size.ws_row ; ++i){
        
        char* row = &textbuffer[i * active_vt->tty.size.ws_col];
        for(unsigned int j = 0; j < active_vt->tty.size.ws_col; ++j){
            
            uint8_t attr = attrbuffer[ (active_vt->tty.size.ws_col * i)  + j];
            bg = term_8_16_colors[ attr  &0xF];
            fg = term_8_16_colors[ (attr & 0xF0) >> 4];
            framebuffer_put_glyph(row[j], 8*j, i * get_glyph_size(), bg, fg);
        }
    }

    return 0;
}