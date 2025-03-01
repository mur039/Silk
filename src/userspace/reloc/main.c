
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "reloc_lib.h"


typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;


int puts(const char * dst){
    write( FILENO_STDOUT, dst, strlen(dst));
    return 0;
}

int putchar(int c){
    return write(FILENO_STDOUT, &c, 1);
}

int getchar(){
    uint8_t ch = 0;
    int ret = read(FILENO_STDIN, &ch, 1);
    return ret ?  ch : -1;
}




int main(int argc, char **argv){

    malloc_init(); //do i need one tho?

    int val = library_func1();
    printf("well val: %x\n", val);
    
    return 0;
}
