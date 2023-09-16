#include <stdarg.h>
#include <16tty.h>

typedef struct term_attribute{
    unsigned int colums;
    unsigned int rows;
    unsigned char cursor_x;
    unsigned char cursor_y;
    union char_attributes {
        struct{
            uint8_t foreground:4;
            uint8_t background:4;   
        };
        uint8_t raw;
    } attr;
    unsigned short * base_addr;
}term_attribute_t;

static term_attribute_t gterm_t;

void get_cursor_pos(){
    short tmp = 0;
    asm volatile( //get cursor position
        "mov $0x3, %ah\n\t"
        "mov $0x0, %bh\n\t"
        "int $0x10    \n\t"
    );
    asm volatile( //get cursor position
        "mov %%dx  ,%0\n\t"
        : "=r" (tmp)
    );
    gterm_t.cursor_x = tmp & 0xFF;
    gterm_t.cursor_y = (tmp >> 8) & 0xFF;
    return;
}

void update_cursor_pos(){
    short a = (gterm_t.cursor_y << 8) | gterm_t.cursor_x ;
    asm volatile( //get cursor position
        "mov %0, %%dx\n\t"
        :
        : "r" (a)
    );
    asm volatile( //get cursor position
        "mov $0x2, %ah\n\t"
        "mov $0x0, %bh\n\t"
        "int $0x10    \n\t"
    );

    return;
}

void scroll(){
    if(gterm_t.cursor_y >= 25){ //gotta scroll honey
        uint16_t * head = (uint16_t *)(gterm_t.base_addr + gterm_t.colums);

        for(uint32_t i = 0; i <(gterm_t.rows - 1) ; ++i){
            for(uint32_t j = 0; j < (gterm_t.colums) ; ++j){
                gterm_t.base_addr[(i * gterm_t.colums) + j] = head[(i * gterm_t.colums) + j];
            }   
        }
        for(uint32_t j = 0; j < (gterm_t.colums) ; ++j){
                gterm_t.base_addr[( gterm_t.rows * gterm_t.colums) + j] = 0xF000;
            }   
        gterm_t.cursor_y = 24;
    }
    
    return;
}

void init_screen(){
    gterm_t.base_addr = (unsigned short *)0xb8000;
    gterm_t.attr.background = 0;
    gterm_t.attr.foreground = 15;
    gterm_t.colums = 80;
    gterm_t.rows = 25;
    get_cursor_pos();
    return;
}


int  printf(char * s, ...){
    va_list va;
    va_start(va, s);

    for (int i = 0; s[i] != 0; i++){
        if(s[i] == '%'){
            //show time
            ++i;
            switch (s[i]){
            
            case 'x':
                ;
                uint32_t xvar = va_arg(va, uint32_t);
                const char hextable[17] = "0123456789abcdef";
                char textBuffer[9];
                uint16_t off = 0;
                for(int i = 0; i < 8; ++i){
                    textBuffer[7 - i] = hextable[(xvar >> (4*i)) & 0xF];
                }

                /*
                for(int i = 0; i < 8; ++i){
                    if(textBuffer[i] == '0'){
                        ++off;
                    }
                }
                */
                puts(&textBuffer[off]);
                for(int i = 0; i<9; ++i ){
                    textBuffer[i] = 0x00;
                }
                break;
            case 'u':
                ;
                unsigned int uvar = va_arg(va, unsigned int);
                unsigned int digitCount = uvar > 0 ? 0 : 1 ;
                for(unsigned int  i = uvar; i != 0; i /= 10 ){++digitCount;};
                
                for(unsigned int i = 0, prev = 0; i < digitCount; ++i){
                    unsigned int now = 0;
                    unsigned int divider = 1;
                    for(unsigned int  j = i + 1 ; j < digitCount ; ++j){divider *= 10;}
                    now = uvar / divider;
                    prev = now;
                    uvar -= prev * divider;
                    putchar('0' + now);

                }
                
                break;
            default:
                continue;
                break;
            }
        }
        else{
            putchar(s[i]);    
        }
        
        
    }
    
    return 0;
}

void puts(char * s){
    while(*s != 0){
        putchar(*s);
        s++;
    }
    return;
}

int  putchar(char c){
    uint16_t val = gterm_t.attr.raw << 8;
    uint32_t addr = (gterm_t.cursor_x +  gterm_t.cursor_y * 80);
    //handling em
    switch (c){
    
    case '\n':
        gterm_t.cursor_x = 0;
        gterm_t.cursor_y += 1;
        break;
    
    default:
        gterm_t.base_addr[addr] = val| c;
        gterm_t.cursor_x += 1;
        if(gterm_t.cursor_x >= 80){
            gterm_t.cursor_x = 0;
            gterm_t.cursor_y += 1;
        }
        break;
    }

    scroll();
    update_cursor_pos();

    return 0;
}