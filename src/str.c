#include <str.h>


int sprintf(char * dest, const char *format, ...){
    va_list args;
    va_start(args, format);
    return va_sprintf(dest, format, args);
}

static const char hexCharacters[17] = "0123456789abcdef";

int va_sprintf(char * dest, const char *format, va_list args){
    // va_list args;
    // va_start(args, format);
    char * former_dest = dest;
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
                uint32_t hex = va_arg(args, uint32_t);
                int non_zero = 0;
                for(int i = 0; i < 8; ++i){
                    int nibble = (hex >> (28 -(4*i))) & 0xF;

                    if(nibble != 0 && non_zero == 0){
                        non_zero = 1;
                    }

                    if(non_zero){
                    *(dest++) = hexCharacters[nibble];

                    }
                }
                if(non_zero == 0) *(dest++) = hexCharacters[0];
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
    return (int)(dest - former_dest);
}



int gprintf(char (* write)(char c), const char *format, ...){
    va_list args;
    va_start(args, format);
    return va_gprintf(write, format, args);
}
int va_gprintf(char (* write)(char c), const char *format, va_list args){
    while(*format != '\0'){ //inside the null-terminated string
        if(*format == '%'){ // %d %x %c %s
            ++format;
            switch (*format)
            {
            case 'u': //32 bit integer has maximum 9,38 = 10 digits
                ;
                char line_buffer[11];
                unsigned int number = va_arg(args, unsigned int);
                int t1, t2;
                t1 = number;
                for(int i = 0; i < 10; ++i){
                    t2 = t1 / 10;
                    t2 *= 10;
                    line_buffer[i] = (t1 - t2);
                    t1 /= 10;
                }

                int found_non_zero = 0;
                for(int i = 9; i >= 0; --i){
                    if( found_non_zero == 0 && line_buffer[i] != 0) found_non_zero = 1;

                    if(found_non_zero || (i == 0 && !found_non_zero)) write('0' + line_buffer[i]);
                }

                break;
            case 'c': //character
                ;
                int ch = va_arg(args, int); //char gets promoted to int 
                // *(dest++) = 
                write(ch & 0xFF);
                break;

            case 's': //string?
                ;
                char * src = va_arg(args, char *);
                for(int i = 0; src[i] != '\0';++i){ //memcpy
                    write(src[i]);
                    }
                break;
            case 'x': //hex output
                //max number is 32-bit so with hexadecimal it
                //can be represented with 8 characters
                ;
                uint32_t hex = va_arg(args, uint32_t);
                int non_zero = 0;
                for(int i = 0; i < 8; ++i){
                    int nibble = (hex >> (28 -(4*i))) & 0xF;
                    if(nibble != 0 && non_zero == 0){
                        non_zero = 1;
                    }

                    if(non_zero || (i == 7 && non_zero == 0)){
                    write(hexCharacters[nibble]);

                    }
                    // *(dest++) = 
                }
                break;
            default:break; //default
            }
        }
        else { //normal
            write(*format);
        }
        
        ++format;
    }
    write('\0');
    va_end(args);
    return 0;
}







size_t strlen(const char * s){
    size_t len ;
    for(len = 0; s[len] != '\0'; len++);
    return len;
}


void * memset(void* s, int c, uint32_t n){
    char * head = (char*)s;
    for(unsigned int  i = 0; i < n; ++i){
        head[i] = c & 0xFF;
    }
    return s;
}

void * memcpy(void * dest, const void * src, uint32_t n){
    char *p1;
    const char *p2;
    p1 = (char*)dest;
    p2 = (const char*)src;

    for (size_t i = 0; i < n; i++){
        p1[i] = p2[i];
    }

    return dest;
}

 int memcmp(const void *s1, const void *s2, size_t n ){
    int result = 0;
    const unsigned char *p1 = s1, *p2 = s2;
    for(size_t i = 0; i < n ; ++i){
        if(p1[i] != p2[i]){
            result = p1[i] - p2[i];
            result = result < 0? -1 : 1;
            break;       
        }
    }
    return result;
 }
