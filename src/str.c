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

                    if(found_non_zero || (i == 0 && !found_non_zero)) 
                        *(dest++) = '0' + line_buffer[i];
                }

                break;

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
                if(!src){ //null pointer

                    const char * null_str = "(null)";
                    for(int i = 0; i < strlen( null_str ); ++i){
                        write(null_str[i]);
                    }
                    break;
                }
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
    
    //don't dereference null pointers
    if(!str1 || !str2)
        return -1;

    for(int i = 0; ; ++i){

        //ugly af but true to the reference i suppose
        if( (str1[i] != '\0' && str2[i] != '\0') && ( str1[i] != str2[i]) ){
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


char * strcpy(char* dst, const char* src){

    //in any of them is null
    if( !dst || !src) {

        return NULL;
    }

    size_t src_len = strlen(src);

    for ( size_t i = 0; i < (src_len + 1); i++){

        dst[i] = src[i];
    }

    return dst;
}

char * strcat(char* dst, const char* src){
    strcpy(dst + strlen(dst), src);
    return dst;

}
#include <pmm.h>
char *strdup(const char *s){
    size_t str_len = strlen(s) + 1;
    char* retval = kcalloc(str_len, 1);
    strcpy(retval, s);
    return retval;
}

void kxxd(const char * src, size_t len){

    for(int i = 0; i < len; ++i){
        
        if( i % 16 == 0 ){

            if(i ){
                
                fb_console_printf("| ");
                for(int j = 0; j < 16; ++j){
                    
                    fb_console_printf("%c", src[i + j - 16 ] <= ' ' ? '.' : src[i + j - 16 ] );
                }
                fb_console_printf("\n");
            }

            fb_console_printf("%x: ", src + i);

        }

        if( (src[i]  & 0xff) < 0xf){
            fb_console_printf(" %x ", src[i]  & 0xff);
        }
        else{
            fb_console_printf("%x ", src[i]  & 0xff);
        }

    }

    
    fb_console_printf("\n");


}


int atoi (const char * str){
    
    int sign = +1;
    int ret_val = 0;
    const  char *digits = str;

    if(str[0] == '-'){
        sign = -1;
        digits++;
    }



    for(size_t i = 0; i < 11 && digits[i] != '\0' ;i++){
        ret_val *= 10;
        ret_val += digits[i] - '0';
    }
    return ret_val * sign;
}
