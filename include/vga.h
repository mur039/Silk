#ifndef _VGA_H_
#define _VGA_H_
#include <stdarg.h>

enum VGA_COLOURS{
    VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15
};


typedef union charAttr{
    struct {
		char character;
		union {
			char attribute;
			struct{
				char background:4;
    			char foreground:4;	
			};
			
		};
	};
	short raw;

} charAttr;

extern charAttr (* videoMemory)[80];
#define textScreen (short *)0xb8000

void initVGATerm();
void update_csr(void);
void scroll(void);
void putch (char c);
void puts(char  * str);
int printf(const char * str, ...);

#endif
