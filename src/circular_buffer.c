#include <circular_buffer.h>



circular_buffer_t circular_buffer_create(size_t buffer_size){
    circular_buffer_t buf;
    buf.max_size = buffer_size;
    buf.base = (uint8_t *)kmalloc(buffer_size);
    buf.write = 0;
    buf.read = 0;
    return buf;
}


void circular_buffer_destroy(circular_buffer_t * cb){
    
    kfree(cb->base);
    cb->base = NULL;
    cb->max_size = 0;
    cb->write = 0;
    cb->read = 0;
}



int circular_buffer_getc(circular_buffer_t * cb){
    if(cb->read == cb->write){
        return -1;    
    }
    else{
        cb->read += 1;
        return cb->base[cb->read - 1];
    }

    return 0;
}

//more like fifo may be i should rename it to be
int circular_buffer_putc(circular_buffer_t * cb, uint8_t val){
    if ( cb->write >= cb->max_size){
        return -1; //buffer full
    } 

    cb->base[cb->write] = val; //problematic
    cb->write++;
    return val;
}

int circular_buffer_write(circular_buffer_t * bf, void * src, uint32_t size, uint32_t nmemb){
    
    for(unsigned int i = 0; i < size*nmemb;++i){
        circular_buffer_putc(bf, ((uint8_t *)src)[i]);
    }
}