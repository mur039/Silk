#include <stdlib.h>
#include <string.h>



int abs(int n){
    return n * (n < 0 ? -1 : 1);
}

/*
well 32-bit due to size can hold values between (-2^31, 2^31) so a 31-bit
 2^31 has ~10.33 digits or 11 digits, characters first character is sign digit the rest is the integer
 for now only decimal is supported no hex or octal
*/
int atoi (const char * str){
    int sign = str[0] == '+'? 1 : -1;
    int ret_val = 0;

    const  char *digits = &str[1];
    for(size_t i = 0; i < 11 && digits[i] != '\0' ;i++){
        ret_val += digits[i] - '0';
        ret_val *= 10;
    }
    return ret_val * sign;
}