#include <string.h>

size_t strlen(const char * str){
    for(size_t i = 0;   ;i++){

        if(str[i] == '\0') 
            return i;
    }
}

int strcmp ( const char * str1, const char * str2 ){
    unsigned char *h1, *h2;
    h1 = (unsigned char *)str1;
    h2 = (unsigned char *)str2;

    for(size_t i = 0;  ; ++i){

        if(h1[i] != h2[i] ){
            return ( h1[i] - h2[i]);
        }

    }

    return 0;
}


void* memset(void * ptr, int value, size_t num){
    unsigned char* head = (unsigned char*)ptr;
    for(size_t i = 0; i < num; ++i ){
        head[i] = value;
    }
    return ptr;
}

int memcmp( const void * ptr1, const void * ptr2, size_t num ){

    unsigned char *h1, *h2;
    h1 = (unsigned char *)ptr1;
    h2 = (unsigned char *)ptr2;

    for (size_t i = 0; i < num; i++){
        
        if(h1[i] != h2[i] ){
            return ( h1[i] - h2[i]);
        }
        
    }
    
    return 0;
}

void * memcpy ( void * destination, const void * source, size_t num ){
    
    unsigned char  *_destination, *_source;

    _destination =   (unsigned char*)destination;
    _source =    (unsigned char*)source;
    for (size_t i = 0; i < num; i++){   
        _destination[i] = _source[i];
    }

    return _destination;
    
}