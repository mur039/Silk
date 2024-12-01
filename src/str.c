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
            case 'u': //32 bit unsigned integer has maximum 9,38 + 1 = 11 digits
                ;
                char line_buffer[11];
                unsigned int number = va_arg(args, unsigned int);
                int t1, t2;
                t1 = number;
                for(int i = 0; i < 11; ++i){
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
    uint8_t * head = (char*)s;
    uint8_t value = c & 0xff;

    for(unsigned int  i = 0; i < n; ++i){
        head[i] = value;
    }
    return s;
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


int is_char_in_str(const char c, const char * str){
    int c_count = 0;
    for(int i = 0; str[i] != '\0'; i++){
        if(str[i] == c){
            c_count++;
        }
    }

    return c_count;
}

int strcmp(const char *str1, const char * str2){
    for(int i = 0; ; ++i){

        //ugly af but true to the reference i suppose
        if( (str1[i] != '0' && str2[i] != '0') && ( str1[i] != str2[i]) ){
            return 1; //?
        }

        if( str1[i] == '\0' && str2[i] != '\0'){
            return -1;
        }

        if( str1[i] != '\0' && str2[i] == '\0'){
            return 1;
        }

        if( str1[i] == '\0' && str2[i] == '\0'){
            return 0;
        }

    }

    return 0;
}


int strncmp(const char *str1, const char * str2, int n){
    for(int i = 0; i < n; ++i){

        //ugly af but true to the reference i suppose
        if( (str1[i] != '0' && str2[i] != '0') && ( str1[i] != str2[i]) ){
            return 1; //?
        }

        if( str1[i] == '\0' && str2[i] != '\0'){
            return -1;
        }

        if( str1[i] != '\0' && str2[i] == '\0'){
            return 1;
        }

        if( str1[i] == '\0' && str2[i] == '\0'){
            return 0;
        }

    }

    return 0;
}