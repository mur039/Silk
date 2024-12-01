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
    
    return f;
}

pipe_t * pipe_get_pntr_from_tar( tar_header_t * header){
    pipe_t * ret = (pipe_t *)&header[1];
    ret = &ret[-1];
    return ret;
}