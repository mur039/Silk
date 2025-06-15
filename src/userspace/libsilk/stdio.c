
#include <stdio.h>
#include <unistd.h>

int FILENO_STDIN = 0;
int FILENO_STDOUT = 1;
int FILENO_STDERR = 2;


static const char hexCharacters[17] = "0123456789abcdef";
static int va_gprintf(char (* write)(char c, void** opt), void* opt, const char *format, va_list args);


int printf (const char * format, ... ){
    va_list arg;
    va_start(arg, format);
    return vprintf(format, arg);
}

static char vprintf_gprintf_wrapper(char c, void** opt){
    write(FILENO_STDOUT, &c, 1);
    return c;
}

int vprintf (const char * format,  va_list arg ){
    return va_gprintf(vprintf_gprintf_wrapper, NULL, format, arg);

}

int sprintf( char * str, const char * format, ... ){
    va_list arg;
    va_start(arg, format);
    return vsprintf(str, format, arg);
}



static char vsprintf_gprintf_wrapper(char c, void** opt){
    char ** str = (char**)opt;
    **str = c;
    *str += 1;
    return c;
};

int vsprintf( char * str, const char * format, va_list arg ){
    return va_gprintf(vsprintf_gprintf_wrapper, str, format, arg);
}








//general printf 
static int va_gprintf(char (* write)(char c, void** opt), void* opt, const char *format, va_list args){
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

                    if(found_non_zero || (i == 0 && !found_non_zero)) write('0' + line_buffer[i], &opt);
                }

                break;
            case 'c': //character
                ;
                int ch = va_arg(args, int); //char gets promoted to int 
                // *(dest++) = 
                write(ch & 0xFF, &opt);
                break;

            case 's': //string?
                ;
                char * src = va_arg(args, char *);
                if(!src){
                    const char null_str[9] = " (null) ";
                    for(int i = 0; i < 9 ;++i){ write(null_str[i], &opt); }    
                }
                for(int i = 0; src[i] != '\0';++i){ //memcpy
                    write(src[i], &opt);
                    }
                break;
            
            case 'p': //pointer output same as hex but preceeds with 0x
                write('0', &opt);
                write('x', &opt);
                
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
                    write(hexCharacters[nibble], &opt);

                    }
                    // *(dest++) = 
                }
                break;
            default:break; //default
            }
        }
        else { //normal
            write(*format, &opt);
        }
        
        ++format;
    }
    write('\0', &opt);
    va_end(args);
    return 0;
}





