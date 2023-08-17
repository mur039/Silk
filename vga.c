#include <system.h>
#include <vga.h>
#include <stdint.h>

int _curs_x, _curs_y;
int termSize[2] = {0}; //height, width
uint8_t attrib = (VGA_COLOR_BLACK << 4) | VGA_COLOR_CYAN;

charAttr (* videoMemory)[80];

void initVGATerm(){ //it will clear screen and set cursor to (0,0)
    videoMemory = (charAttr (*)[80])0xb8000;
    termSize[0] = 25, termSize[1] = 80;
    _curs_x = 0, _curs_y = 0;

    charAttr blank = {.background = VGA_COLOR_BLACK, .foreground = VGA_COLOR_LIGHT_CYAN, .character = '\0'};
    memsetw(videoMemory, blank.raw , 80*25*2);
	update_csr();
    return;
}   

void update_csr(void){ //more like update
    unsigned int temp;
    temp = _curs_y * 80 + _curs_x;
    outportb(0x3D4, 14);
    outportb(0x3D5, temp >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, temp);
}

inline void scroll(void){ //i know the size so
    if(_curs_y >= 25){ //do scrolling
        for(int i = 0; i< 25; ++i){
            memcpy(videoMemory[i], videoMemory[i + 1], 80 * sizeof(charAttr));
        }
        _curs_y = 24;
        update_csr();
    }
    return;
}

void putch(char c){

    //unsigned att = attrib << 8;

    if(c >=' '){ //printable character
        videoMemory[_curs_y][_curs_x] = (charAttr) {.attribute = attrib, .character = c};
        _curs_x++;
    }
    else{
        
        switch (c)
        {
        case 0x08: if(_curs_x != 0){ _curs_x -= 1;} break; //backspace
        case 0x09: _curs_x = (_curs_x + 8) & ~7;break;//tab also cursx divisible by 8
        case '\n': _curs_x = 0;++_curs_y;break; //newline
        case '\r':_curs_x = 0;break; //carriage return
        default:break; //do nothing
        }

        

    }
    /* If the cursor has reached the edge of the screen's width, we
    *  insert a new line in there */
    if(_curs_x >= 80)
    {
        _curs_x = 0;
        _curs_y++;
    }

    /* Scroll the screen if needed, and finally move the cursor */
    scroll();
    update_csr();
    return;
}

void puts(char *text)
{
    int i;
    for (i = 0; i < strlen(text); i++)
    {
        putch(text[i]);
    }
}

const char charTable[][16] = {
	"0123456789",
	"0123456789ABCDEF"
};
int printf(const char * str, ...){
    va_list args;
    va_start(args, str);
    while(*str){
        if(*str == '%'){
            switch (*(++str))
            {
            case 'b':
                ; //label dan sonra declretation olamaz
                unsigned int cArg = va_arg(args, unsigned int) & 0xFF; // 32-bit olduğundan stack'e 4 byte şeklide pushlandı char->int
                for(int i = 0; i < 8; ++i){
                    putch( (cArg & (0b10000000 >> i))? '1' : '0');
                }
                break;
            case 'p':
                puts("0x");
                __attribute__ ((fallthrough));
            case 'x': //unsigned int  // 'x': label dan son declration yapamazsın!
                ;
                unsigned int xArg = va_arg(args, unsigned int); // bu yüzden ; geldi başa
                unsigned int shift = 28;    
                for(unsigned int i = 0, j ; i < 8; ++i){ //soldan sağa olduğu için
                    j = (xArg & (0xF << shift)) >> shift;
                    if(j | 1){putch(charTable[1][j]);}
                    shift -= 4; 
                }

                break;
            
            default:
                break;
            }
        }
        else{putch(*str);}
        str++;
    }
    va_end(args);
    return 0;
}