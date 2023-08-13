#include <system.h>
unsigned char *memcpy(void *dest, const void *src, int count){
    while(count--){
        ((unsigned char*)dest)[count] = ((unsigned char*)src)[count];
    }
    return dest;
}

unsigned char *memset(void *dest, unsigned char val, int count){
    while(count--){
        ((unsigned char*)dest)[count] = val;
    }
    return dest;
}

unsigned short *memsetw(void *dest, unsigned short val, int count){
    while(count--){
        ((unsigned char*)dest)[count] = val;
    }
    return dest;
}

int strlen(const char *str){
    if(str == NULL){return -1;}; //return is given str addr is NULL
    int _length = 0;
    for(; str[_length] != 0; ++_length){}
    return _length;

}

void outportb (unsigned short _port, unsigned char _data){ asm volatile ( "outb %0, %1" : : "a"(_data), "Nd"(_port) );}
void outportw (unsigned short _port, unsigned short _data){asm volatile ("outw %0, %1" : : "a"(_data), "Nd"(_port));}

uint8_t inportb (unsigned short _port){
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(_port) );
    return ret;
}
uint16_t inportw(unsigned short _port){
    uint16_t ret;
    asm volatile ( "inw %1, %0"
                   : "=a"(ret)
                   : "Nd"(_port) );
    return ret;
}
