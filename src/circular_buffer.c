#include <circular_buffer.h>



circular_buffer_t circular_buffer_create(size_t buffer_size){
    circular_buffer_t buf;
    buf.max_size = buffer_size;

    buf.base = (uint8_t *)kmalloc(buffer_size);
    if(!buf.base){
        error("kmalloc failed");
    }
    buf.write = 0;
    buf.read = 0;
    return buf;
}


void circular_buffer_destroy(circular_buffer_t * cb){
    
    if(!cb) return;

    if(cb->base)
        kfree(cb->base);
    
    cb->base = NULL;
    cb->max_size = 0;
    cb->write = 0;
    cb->read = 0;
}



int circular_buffer_getc(circular_buffer_t * cb){
    
    if(circular_buffer_avaliable(cb) == 0){
        return -1;    
    }
    else{
        cb->read = (cb->read + 1) % cb->max_size; 
        return cb->base[cb->read - 1];
    }

    // return 0;
}

//more like fifo may be i should rename it to be
int circular_buffer_putc(circular_buffer_t * cb, uint8_t val){
    if ( circular_buffer_avaliable(cb) >= (int)(cb->max_size - 1)){
        return -1; //buffer full
    } 

    cb->base[cb->write] = val; //problematic
    cb->write = (cb->write + 1) % cb->max_size; 

    return val;
}

int circular_buffer_write(circular_buffer_t * bf, void * src, uint32_t size, uint32_t nmemb){
    
    for(unsigned int i = 0; i < size*nmemb;++i){
        circular_buffer_putc(bf, ((uint8_t *)src)[i]);
    }
}

int circular_buffer_read(circular_buffer_t * bf, void * dbuf, uint32_t size, uint32_t nmemb){
    u8 * dbuf8 = dbuf;
    for(unsigned int i = 0; i < size*nmemb;++i){
        int ch = circular_buffer_getc(bf);
        if(  ch == -1){ //no data in buffer
            return i; //return what we read
        }

        dbuf8[i] = ch;
    }

    return size*nmemb;
}


int circular_buffer_avaliable(const circular_buffer_t* cb){
    size_t read = cb->read;
    size_t write = cb->write;
    size_t msize = cb->max_size;

    if(write > read){
        return write - read ;
    }
    else if ( write < read){
        return msize + write - read;
    
    }
    else{ //write == read
        return 0;
    }
}