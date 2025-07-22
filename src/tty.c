#include <tty.h>
#include <syscalls.h>

tty_t tty_create_device();


int tty_ld_write(tty_t* tty, const char* str, size_t nbytes){

    if(!tty->refcount && !tty->session){
        //no need to read and write
        return 0;    
    }

    if(!(tty->termio.c_lflag & ICANON) ){ //raw mode
        
        process_wakeup_list(&tty->read_wait_queue);
        circular_buffer_write(&tty->read_buffer, (void*)str, 1, nbytes);
        if(tty->termio.c_lflag & ECHO){
            
            if(tty->driver && tty->driver->write)  tty->driver->write(tty, str, nbytes);
        }

        return nbytes;
    }


    for(const char* ch = str; ch < (str + nbytes); ch++){
        size_t linebuffer_size = 0;
        switch(*ch){

            case CTRL('c'): 
            //discard the linebuffer
            tty->linebuffer.read = 0;
            tty->linebuffer.write = 0;
            
            if(tty->termio.c_lflag & ECHO && tty->driver && tty->driver->write){
                tty->driver->write(tty, "^C", 2); //echo it to the screen
            }

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

            case 127:
            case '\b': 
            if(!circular_buffer_avaliable(&tty->linebuffer)) 
                break;
    
            if(tty->linebuffer.write > tty->linebuffer.read){

                tty->linebuffer.write--;
            }
            else if(tty->linebuffer.write < tty->linebuffer.read){
                
                tty->linebuffer.write = (tty->linebuffer.write--) % tty->linebuffer.max_size;
            }
            if(tty->termio.c_lflag & ECHO && tty->driver && tty->driver->write){
                tty->driver->write(tty, ch, 1); //echo it to the screen
            }
            break;

            case '\r':
            case '\n':
            ;
            int nch = *ch;
            if(nch == '\r' && tty->termio.c_iflag & ICRNL){
                nch = '\n';
            }

            circular_buffer_putc(&tty->linebuffer, nch);
            linebuffer_size = circular_buffer_avaliable(&tty->linebuffer);
            
            for(size_t i = 0; i < linebuffer_size; ++i){
                circular_buffer_putc(&tty->read_buffer, circular_buffer_getc(&tty->linebuffer));
            }
            process_wakeup_list(&tty->read_wait_queue);
            if(tty->termio.c_lflag & ECHO && tty->driver && tty->driver->write){
                tty->driver->write(tty, (const char*)&nch, 1); //echo it to the screen
            }
            break;

            default:
            circular_buffer_putc(&tty->linebuffer, *ch);
            if(tty->termio.c_lflag & ECHO && tty->driver && tty->driver->write){
                tty->driver->write(tty, ch, 1); //echo it to the screen
            }
            break;
        }
        
    }

    return nbytes;
}

void tty_open(struct fs_node* fnode, int read, int write){
    
    tty_t* tty = ((device_t*)fnode->device)->priv;
    
    
    if(tty->driver && tty->driver->open){
        tty->driver->open(tty);
    }
    
    
    tty->refcount++;

    //so that so session leader steals the terminal
    if( !tty->session  &&  !current_process->ctty){
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
        
        list_insert_end(&tty->read_wait_queue, current_process);
        return -1;

    }
    else{ //raw mode then just give em john
        size_t remaining = circular_buffer_avaliable(&tty->read_buffer);
        if(remaining < size){ //not enough

            list_insert_end(&tty->read_wait_queue, current_process);
            return -1;
        }

        return circular_buffer_read(&tty->read_buffer, buffer, 1, size);;

    }
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