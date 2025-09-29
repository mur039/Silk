#include <tty.h>
#include <syscalls.h>

tty_t tty_create_device();


int tty_ld_putchar(tty_t* tty, int ch){
    

    if(ch == '\r'){
        if (tty->termio.c_iflag & ICRNL){

            ch = '\n';
        }
        else if (tty->termio.c_iflag & IGNCR){
            return 0;
        }
    }
    else if(ch == '\n'){
        if (tty->termio.c_iflag & INLCR){

            ch = '\r';
        }
    }


    switch(ch){

        // case CTRL('@'):__attribute__((fallthrough));
        // case CTRL('A'):__attribute__((fallthrough));
        // case CTRL('B'):__attribute__((fallthrough));
        // case CTRL('E'):__attribute__((fallthrough));
        // case CTRL('F'):__attribute__((fallthrough));
        // case CTRL('G'):__attribute__((fallthrough));
        // case CTRL('I'):__attribute__((fallthrough));
        // case CTRL('K'):__attribute__((fallthrough));
        // case CTRL('L'):__attribute__((fallthrough));
        // case CTRL('N'):__attribute__((fallthrough));
        // case CTRL('O'):__attribute__((fallthrough));
        // case CTRL('P'):__attribute__((fallthrough));
        // case CTRL('Q'):__attribute__((fallthrough));
        // case CTRL('R'):__attribute__((fallthrough));
        // case CTRL('S'):__attribute__((fallthrough));
        // case CTRL('T'):__attribute__((fallthrough));
        // case CTRL('U'):__attribute__((fallthrough));
        // case CTRL('V'):__attribute__((fallthrough));
        // case CTRL('W'):__attribute__((fallthrough));
        // case CTRL('X'):__attribute__((fallthrough));
        // case CTRL('Y'):__attribute__((fallthrough));
        // case CTRL('['):__attribute__((fallthrough));
        // case CTRL('\\'):__attribute__((fallthrough));
        // case CTRL(']'):__attribute__((fallthrough));
        // case CTRL('^'):__attribute__((fallthrough));
        // case CTRL('_'):__attribute__((fallthrough));
        
        // break;
        
        case CTRL('Z'):
            if(tty->termio.c_lflag & ICANON){
                if(tty->termio.c_lflag & ISIG){

                    process_send_signal_pgrp(tty->fg_pgrp, SIGSTOP);
                }

                process_wakeup_list(&tty->read_wait_queue);
            }
            else{
                circular_buffer_putc(&tty->read_buffer, ch);
                process_wakeup_list(&tty->read_wait_queue);
                if(tty->termio.c_lflag & (ECHO)){
                    tty->driver->write(tty, "^Z", 2);
                }
            }
        break;

        case CTRL('C'): 
            if(tty->termio.c_lflag & ICANON){
                if(tty->termio.c_lflag & ISIG) 
                    process_send_signal_pgrp(tty->fg_pgrp, SIGINT);

                process_wakeup_list(&tty->read_wait_queue);
            }
            else{
                circular_buffer_putc(&tty->read_buffer, ch);
                process_wakeup_list(&tty->read_wait_queue);
                if(tty->termio.c_lflag & (ECHO)){
                    tty->driver->write(tty, "^C", 2);
                }
            }
            
            break;
        break;
        case CTRL('D'):
            if(tty->termio.c_lflag & ICANON){ //EOF
            size_t lbsize = circular_buffer_avaliable(&tty->linebuffer);
            for(size_t i = 0; i < lbsize; ++i){
                circular_buffer_putc(&tty->read_buffer, circular_buffer_getc(&tty->linebuffer));
            }
            tty->eof_pending = 1;
            process_wakeup_list(&tty->read_wait_queue);
        }
        else{
            circular_buffer_putc(&tty->read_buffer, ch);
            process_wakeup_list(&tty->read_wait_queue);
            if(tty->termio.c_lflag & (ECHO)){
                tty->driver->write(tty, "^D", 2);
            }
        }

        break;


        case 127:
        case CTRL('H'): //'\b'
            if(tty->termio.c_lflag & ICANON){
                int aux = circular_buffer_peek_last(&tty->linebuffer);
                if(aux < 0){ //no data
                    break;
                }

                if(tty->linebuffer.write > tty->linebuffer.read){
                    tty->linebuffer.write--;
                }
                else if(tty->linebuffer.write < tty->linebuffer.read){
                    tty->linebuffer.write = (tty->linebuffer.write--) % tty->linebuffer.max_size;
                }

                if(tty->termio.c_lflag & ECHO){
                    if( aux < 32 && aux != '\n' && aux != '\r'  && aux != '\t'  ){
                        tty->driver->write(tty, "\b", 1); //extra backspace
                    }
                    tty->driver->write(tty, "\b", 1); //extra backspace
                }
            }
            else{
                circular_buffer_putc(&tty->read_buffer, ch);
                process_wakeup_list(&tty->read_wait_queue);
                if(tty->termio.c_lflag & (ECHO | ECHONL)){
                    tty->driver->write(tty, "^H", 2);
                }
            }
        break;

        
        case CTRL('J'):  //'\n'
            if(tty->termio.c_lflag & ICANON){
                circular_buffer_putc(&tty->linebuffer, ch);
                size_t linebuffer_size = circular_buffer_avaliable(&tty->linebuffer);
            
                for(size_t i = 0; i < linebuffer_size; ++i){
                    circular_buffer_putc(&tty->read_buffer, circular_buffer_getc(&tty->linebuffer));
                }
                
                if(tty->termio.c_lflag & (ECHONL)){
                    tty->driver->put_char(tty, '\n');
                }
                process_wakeup_list(&tty->read_wait_queue);
            }
            else{
                circular_buffer_putc(&tty->read_buffer, ch);
                process_wakeup_list(&tty->read_wait_queue);
                
                if(tty->termio.c_lflag & (ECHO)){
                    if(tty->termio.c_oflag & ONLCR){
                        ch = '\r';
                    }
                    tty->driver->put_char(tty, '^');
                    tty->driver->put_char(tty, '@' + ch);
                }
                else if(tty->termio.c_lflag & (ECHONL)){
                    tty->driver->put_char(tty, '\n');
                }
            }
        break;

        
        case CTRL('M'): //'\r'
            if(tty->termio.c_lflag & ICANON){
                circular_buffer_putc(&tty->linebuffer, ch);
            }
            else{
                circular_buffer_putc(&tty->read_buffer, ch);
                process_wakeup_list(&tty->read_wait_queue);
                
                if(tty->termio.c_lflag & (ECHO)){
                    tty->driver->put_char(tty, '^');
                    tty->driver->put_char(tty, '@' + ch);
                }
            }
        break;


        default:
            if(tty->termio.c_lflag & ICANON){
                circular_buffer_putc(&tty->linebuffer, ch);
            }
            else{
                circular_buffer_putc(&tty->read_buffer, ch);
                process_wakeup_list(&tty->read_wait_queue);
            }

            if(tty->termio.c_lflag & ECHO){
                if(ch >= 32){

                    tty->driver->put_char(tty, ch);
                }
                else{
                    tty->driver->put_char(tty, '^');
                    tty->driver->put_char(tty, '@' + ch);
                }
            }
        break;
    }


}





int tty_ld_write(tty_t* tty, const char* str, size_t nbytes){

    if(!tty->refcount && !tty->session){
        return 0;    
    }

    for(size_t i = 0; i < nbytes; ++i){

        tty_ld_putchar(tty, str[i]);
    }
    return nbytes;

    if(!(tty->termio.c_lflag & ICANON) ){ //raw mode
        
        
        for(const char* ch = str; ch < (str + nbytes); ch++){
            int nch = *ch;
            switch(*ch){
            case '\r':
            case '\n':
                ;
                if(nch == '\r' ){
                    if(tty->termio.c_iflag & IGNCR){
                        break;
                    }

                    if( tty->termio.c_iflag & ICRNL){

                        nch = '\n';
                    }
                }
            
                if(nch == '\n' && tty->termio.c_iflag & INLCR){
                    nch = '\r';
                }
            }



            
            circular_buffer_write(&tty->read_buffer, (void*)&nch, 1, 1);
            if(tty->termio.c_lflag & ECHO){

                if(tty->driver && tty->driver->write){

                    tty->driver->write(tty, (const char*)&nch, 1);
                }
            }
            else if(tty->termio.c_lflag & ECHONL){
                tty->driver->put_char(tty, '\n');
            }
        }
        
        process_wakeup_list(&tty->read_wait_queue);
        return nbytes;
    }

    int aux = 0;
    for(const char* ch = str; ch < (str + nbytes); ch++){
        size_t linebuffer_size = 0;
        switch(*ch){

            case CTRL('c'): 
            //discard the linebuffer
            tty->linebuffer.read = 0;
            tty->linebuffer.write = 0;

            if(tty->termio.c_lflag & ISIG) 
                process_send_signal_pgrp(tty->fg_pgrp, SIGINT);

            break;

            case CTRL('d'):
            linebuffer_size = circular_buffer_avaliable(&tty->linebuffer);
            for(size_t i = 0; i < linebuffer_size; ++i){
                circular_buffer_putc(&tty->read_buffer, circular_buffer_getc(&tty->linebuffer));
            }
            tty->eof_pending = 1;
            process_wakeup_list(&tty->read_wait_queue);
            break;
            
            case CTRL('z'):
                process_send_signal_pgrp(tty->fg_pgrp, SIGSTOP);
            break;

            case 127:
            case '\b': 
                aux = circular_buffer_peek_last(&tty->linebuffer);
                if(aux < 0){ //no data
                    continue;
                }
            

            if(tty->linebuffer.write > tty->linebuffer.read){

                tty->linebuffer.write--;
            }
            else if(tty->linebuffer.write < tty->linebuffer.read){
                
                tty->linebuffer.write = (tty->linebuffer.write--) % tty->linebuffer.max_size;
            }

            if( aux < 32 && aux != '\n' && aux != '\r'  && aux != '\t'  ){
                if(tty->termio.c_lflag & ECHO && tty->driver && tty->driver->write){
        
                    tty->driver->write(tty, "\b", 1); //extra backspace
                }
            }
            
            break;

            case '\r':
            case '\n':
            ;
            int nch = *ch;
            if(nch == '\r' ){
                if(tty->termio.c_iflag & IGNCR){
                    break;
                }

                if( tty->termio.c_iflag & ICRNL){

                    nch = '\n';
                }
            }
            
            if(nch == '\n' && tty->termio.c_iflag & INLCR){
                nch = '\r';
            }

            circular_buffer_putc(&tty->linebuffer, nch);
            linebuffer_size = circular_buffer_avaliable(&tty->linebuffer);
            
            for(size_t i = 0; i < linebuffer_size; ++i){
                circular_buffer_putc(&tty->read_buffer, circular_buffer_getc(&tty->linebuffer));
            }
            process_wakeup_list(&tty->read_wait_queue);
            break;




            default:
            circular_buffer_putc(&tty->linebuffer, *ch);
            break;
        }


        if(tty->termio.c_lflag & ECHO && tty->driver && tty->driver->write){
            
            if(*ch == '\n' || *ch == '\r' || *ch =='\b' || *ch == '\t' ||*ch >= 32){
                
                tty->driver->write(tty, ch, 1); //echo it to the screen
            }
            else if(*ch < 32){
                char ctrl_template[3] = "^@";
                ctrl_template[1] += *ch;
                tty->driver->write(tty, ctrl_template, 2); //echo it to the screen
            }
            
        }
        
    }

    return nbytes;
}

void tty_open(struct fs_node* fnode, int flags){
    
    tty_t* tty = ((device_t*)fnode->device)->priv;
    
    
    if(tty->driver && tty->driver->open){
        tty->driver->open(tty);
    }
    
    
    tty->refcount++;

    //so that so session leader steals the terminal
    if( !tty->session  &&  !current_process->ctty && !(flags & O_NOCTTY)){
        
        current_process->ctty = tty;
        tty->session = current_process->sid;
        tty->fg_pgrp = current_process->pgid;
    }
    return;
};

void tty_close(struct fs_node* fnode){
    tty_t* tty = ((device_t*)fnode->device)->priv;

    if(tty->refcount > 0){

        tty->refcount--;
        
        if(tty->driver && tty->driver->close){
            tty->driver->close(tty);
        }

    }
    

    return;
};



uint32_t tty_write(struct fs_node* fnode, uint32_t offset, uint32_t size, uint8_t* buffer){
    tty_t* tty = ((device_t*)fnode->device)->priv;

    if(tty->driver->write)
        tty->driver->write(tty, buffer, size);

    return size;
}

uint32_t tty_read(struct fs_node* fnode, uint32_t offset, uint32_t size, uint8_t* buffer){
    tty_t* tty = ((device_t*)fnode->device)->priv;

    //if canonical then should read up to readline or size
    if(tty->termio.c_lflag & ICANON){ 

        if(tty->eof_pending == 1){
            size_t remaining = circular_buffer_avaliable(&tty->read_buffer);
            if(remaining) 
                circular_buffer_read(&tty->read_buffer, buffer, 1, remaining);
            tty->eof_pending = 0;
            return remaining;
        }
        
        size_t remaining = circular_buffer_avaliable(&tty->read_buffer);
        if(remaining > 0){
            
            if(remaining > size){
                remaining = size;
            }

            circular_buffer_read(&tty->read_buffer, buffer, 1, remaining);
            return remaining;
        }
        
        //prevent duplition in the list
        if(!list_find_by_val(&tty->read_wait_queue, current_process)){
            list_insert_end(&tty->read_wait_queue, current_process);
        }
        return -1;

    }
    else{ //raw mode then just give em john

        struct termios* io = &tty->termio;
        int minimumbyte = io->c_cc[VMIN];
        size_t milis = io->c_cc[VTIME] * 100;
        timer_event_t* ev = NULL;

        
        if(milis > 0){ //timeout exists
            if(minimumbyte == 0){
                size_t avaliable = circular_buffer_avaliable(&tty->read_buffer);
                if(avaliable){
                    return circular_buffer_read(&tty->read_buffer, buffer, 1, size);
                }

                
                int timer_idx = timer_register(milis, 0, NULL, NULL);
                ev = timer_get_by_index(timer_idx);
                sleep_on(&ev->wait_queue);
                timer_destroy_timer(ev);
                return circular_buffer_read(&tty->read_buffer, buffer, 1, size);
            }

        }
        else{ //just block darling

            if(minimumbyte == 0){ //return what's there
                //the circular buffer returns what's inside, doesn't block
                return circular_buffer_read(&tty->read_buffer, buffer, 1, size);
            }
            else{ //wait for some amount
                interruptible_sleep_on(&tty->read_wait_queue, circular_buffer_avaliable(&tty->read_buffer) >= minimumbyte);
                return circular_buffer_read(&tty->read_buffer, buffer, 1, size);
            }
        }



        // while(1){
        //     size_t avaliable = circular_buffer_avaliable(&tty->read_buffer);

        //     if(minimumbyte == 0 && avaliable > 0){
        //         return circular_buffer_read(&tty->read_buffer, buffer, 1, size);
        //     }

        //     if(avaliable >= minimumbyte){
        //         return circular_buffer_read(&tty->read_buffer, buffer, 1, size);
        //     }

        //     //not enough
        //     if(milis > 0){
        //         if(!ev){ //say if we already have a timer or not
        //             int timerindex = timer_register(milis, 0, NULL, NULL);
        //             if(timerindex >= 0){ //a valid timer
        //                 ev = timer_get_by_index(timerindex);
        //                 list_insert_end(&ev->wait_queue, current_process);
        //             }
        //         }
        //     }
            
        //     sleep_on(&tty->read_wait_queue);

        //     //did timer expired?
        //     if(ev && !timer_is_pending(ev)){
        //         timer_destroy_timer(ev);
        //         avaliable = circular_buffer_avaliable(&tty->read_buffer);
        //         return circular_buffer_read(&tty->read_buffer, buffer, 1, avaliable);
        //     }
        // }
    }
}




short tty_poll(struct fs_node *fn, struct poll_table* pt){
    
    tty_t* tty = ((device_t*)fn->device)->priv;
    poll_wait(fn, &tty->read_wait_queue, pt);
    
    short mask = 0;
    
    if( circular_buffer_avaliable(&tty->read_buffer) ){
        mask |= POLLIN;
    }

    return mask;
}


int tty_ioctl(fs_node_t* fnode, unsigned long op, void* argp){

    //no controlling terminal
    if(!current_process->ctty){
        return -ENOTTY;
    }


    tty_t* tty = ((device_t*)fnode->device)->priv;
    if(current_process->ctty != tty){ //fd not associated with this tty
        return -ENOTTY;
    }

    if(tty->session != current_process->sid){
        return -ENOTTY;
    }

    

    struct termios* term;
    struct winsize *winsize;
    switch(op){
        case TCGETS:
            if(!is_virtaddr_mapped(argp)) return -EFAULT;
            
            term = argp;
            *term = tty->termio;
        break;

        case TCSETS:
        case TCSETSW:
        case TCSETSF:
            if(!is_virtaddr_mapped(argp)){
                return -EFAULT;
            }
            term = argp;
            tty->termio = *term;
        break;

        case TCGETA:
        break;
        case TCSETA:
        break;
        case TCSETAW:
        break;
        case TCSETAF:
        break;
        case TCSBRK:
        break;
        case TCXONC:
        break;
        case TCFLSH:
            tty->read_buffer.read = 0;
            tty->read_buffer.write = 0;
        break;
        case TIOCEXCL:
        break;
        case TIOCNXCL:
        break;
        case TIOCSCTTY:
        break;
        case TIOCGPGRP:

            *(pid_t*)argp = tty->fg_pgrp;
        break;

        case TIOCSPGRP:
        {
            pid_t pgrp = (pid_t)argp;    
            tty->fg_pgrp = pgrp;
        }
        break;
        case TIOCOUTQ:
        break;
        case TIOCSTI:
        break;

        case TIOCGWINSZ:
            if(!is_virtaddr_mapped(argp)) return -EFAULT;
            winsize = argp;
            winsize->ws_col = tty->size.ws_col;
            winsize->ws_row = tty->size.ws_row;
        break;
        
        case TIOCSWINSZ:
            if(!is_virtaddr_mapped(argp)) return -EFAULT;
            winsize = argp;
            tty->size.ws_col = winsize->ws_col ;
            tty->size.ws_row = winsize->ws_row ;
            // process_send_signal_pgrp(tty->fg_pgrp, SIGWINCH);
        break;
        case TIOCMGET:
        break;
        case TIOCMBIS:
        break;
        case TIOCMBIC:
        break;
        case TIOCMSET:
        break;
        case TIOCGSOFTCAR:
        break;
        case TIOCSSOFTCAR:
        break;

        default:
            return -ENOTTY; //ENOTTY
        break;
    }

    pmm_alloc_is_chain_corrupted();    
    return 0;
}




int install_tty(){
    //TODO implement /dev/tty device
    return 0;
}
