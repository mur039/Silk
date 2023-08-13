#include <system.h>
#include <vga.h>


int _curs_x, _curs_y;
int termSize[2] = {0}; //height, width
uint8_t attrib = (VGA_COLOR_BLACK << 4) | VGA_COLOR_CYAN;

void initVGATerm(){ //it will clear screen and set cursor to (0,0)
    termSize[0] = 25, termSize[1] = 80;
    _curs_x = 0, _curs_y = 0;
    memsetw((short * )(textScreen), (( VGA_COLOR_BLACK<< 4| VGA_COLOR_WHITE ) << 8 ) | '\0', termSize[0]*termSize[1]);
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

void scroll(void){ //i know the size so
    
    return;
}

void putch(unsigned char c){

    unsigned short *where;
    unsigned att = attrib << 8;

    /* Handle a backspace, by moving the cursor back one space */
    if(c == 0x08)
    {
        if(_curs_x != 0) _curs_x--;
    }
    /* Handles a tab by incrementing the cursor's x, but only
    *  to a point that will make it divisible by 8 */
    else if(c == 0x09)
    {
        _curs_x = (_curs_x + 8) & ~(8 - 1);
    }
    /* Handles a 'Carriage Return', which simply brings the
    *  cursor back to the margin */
    else if(c == '\r')
    {
        _curs_x = 0;
    }
    /* We handle our newlines the way DOS and the BIOS do: we
    *  treat it as if a 'CR' was also there, so we bring the
    *  cursor to the margin and we increment the 'y' value */
    else if(c == '\n')
    {
        _curs_x = 0;
        _curs_x++;
    }
    /* Any character greater than and including a space, is a
    *  printable character. The equation for finding the index
    *  in a linear chunk of memory can be represented by:
    *  Index = [(y * width) + x] */
    else if(c >= ' ')
    {
        where = textScreen + (_curs_y * 80 + _curs_x);
        *where = c | att;	/* Character AND attributes: color */
        _curs_x++;
    }

    /* If the cursor has reached the edge of the screen's width, we
    *  insert a new line in there */
    if(_curs_x >= 80)
    {
        _curs_x = 0;
        _curs_y++;
    }

    /* Scroll the screen if needed, and finally move the cursor */
    //scroll();
    update_csr();
    return;
}

void puts(unsigned char *text)
{
    int i;

    for (i = 0; i < strlen(text); i++)
    {
        putch(text[i]);
    }
}
