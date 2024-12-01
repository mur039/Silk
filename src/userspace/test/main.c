
#include <stdint-gcc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int puts(const char *str){
    return write(0, str, strlen(str));
}

int main( int argc, char* argv[]){

    unsigned int * head;
    asm volatile( 
        "mov %%ebp, %0"
        : "=r"(head)
        );

    char line_buffer[64];
    sprintf(line_buffer, "argc: %u argv:%p  \n", argc, argv);
    puts(line_buffer);

    for(int i = 0 ; i < argc; ++i){
        sprintf(line_buffer, "argv[%u]: %s  \n", i, argv[i]);
        puts(line_buffer);
    }
    
    
    return 0;
}
