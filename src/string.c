#include <string.h>

int sprintf(char * dest, const char *format, ...){
    va_list args;
    va_start(args, format);
    while(*format != '\0'){ //inside the null-terminated string
        if(*format == '%'){ // %d %x %c %s
            ++format;
            switch (*format)
            {
            case 'c': //character
                ;
                int ch = va_arg(args, int); //char gets promoted to int 
                *(dest++) = ch & 0xFF;
                break;

            case 's': //string?
                ;
                char * src = va_arg(args, char *);
                for(int i = 0; src[i] != '\0';++i){ //memcpy
                    *(dest++) = src[i];
                    }
                break;
            case 'x': //hex output
                //max number is 32-bit so with hexadecimal it
                //can be represented with 8 characters
                ;
                const char hexCharacters[17] = "0123456789abcdef";
                uint32_t hex = va_arg(args, uint32_t);
                for(int i = 0; i < 8; ++i){
                    *(dest++) = hexCharacters[(hex >> (28 -(4*i))) & 0xF];
                }
                break;
            default:break; //default
            }
        }
        else { //normal
            *(dest++) = *format;
        }
        
        ++format;
    }
    *dest = '\0';
    va_end(args);
    return 0;
}

void * memset(void* s, int c, uint32_t n){
    char * head = (char*)s;
    for(unsigned int  i = 0; i < n; ++i){
        head[i] = c & 0xFF;
    }
    return s;
}