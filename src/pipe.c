#include <pipe.h>



file_t pipe_create(size_t size){
    file_t f;
    circular_buffer_t cb =  circular_buffer_create(size); //16
    
    //will use tar header as data
    tar_header_t * tar = kcalloc(1, sizeof(tar_header_t) );
    f.f_inode = tar;
    f.f_inode->typeflag[0] = FIFO_SPECIAL_FILE + '0';
    
  
    /*
    come to think of it using name fielf is not smart if named pipes are implemented
    in tar header meaningfull data is in first 345 bytes and total structure is 512 byte
    */
    
    
    pipe_t * ph = (pipe_t*)&tar[1];
    ph = &ph[-1];
    ph->cb = cb;
    ph->read_refcount = 1;
    ph->write_refcount = 1;
    
    f.ops.priv = ph;
    f.ops.read = pipe_read;
    f.ops.write = pipe_write;
    f.ops.open = pipe_open;
    f.ops.close = pipe_close;

    return f;
}

pipe_t * pipe_get_pntr_from_tar( tar_header_t * header){
    pipe_t * ret = (pipe_t *)&header[1];
    ret = &ret[-1];
    return ret;
}

int32_t pipe_read(uint8_t * buffer, uint32_t offset, uint32_t len, void *d){
    pipe_t *bf = d;
    (void)offset;
    (void)len;//ignored

    if(!bf->write_refcount){
        return -1;
    }
    int ch = circular_buffer_getc(&bf->cb);
    
    if(ch == -1){
        return -1;
    }
    *buffer = (uint8_t)ch;
    return 1;

}

int32_t pipe_write(uint8_t * buffer, uint32_t offset, uint32_t len, void *d){
    pipe_t *bf = d;
    (void)offset;
    (void)len;//ignored


    if(!bf->read_refcount){
        fb_console_printf("no reading at the other end\n");
        return -1;
    }
    

    circular_buffer_write(&bf->cb, buffer, 1, len);
    return len;

}

uint8_t pipe_close(int t, void *d){
    return 0;
    pipe_t *bf = d;
    
    if(!t){ //read
        bf->read_refcount--;
        fb_console_printf("decremented read_count\n");
        if(bf->read_refcount < 0) bf->read_refcount = 0;
    }
    else{
        bf->write_refcount--;
        fb_console_printf("decremented write_count\n");
        if(bf->write_refcount < 0) bf->write_refcount = 0;
    }

    fb_console_printf("current refcount r/w : %u/%u\n",bf->read_refcount, bf->write_refcount);    
    return 0;
}


uint8_t pipe_open(int t, void *d){
    
    pipe_t *bf = d;
    if(!t){ //read
        fb_console_printf("Incremented read_count\n");
        bf->read_refcount++;
    }
    else{
        fb_console_printf("Incremented write_count\n");
        bf->write_refcount++;
    }

    fb_console_printf("current refcount r/w : %u/%u\n",bf->read_refcount, bf->write_refcount);
    
    return 0;
}




