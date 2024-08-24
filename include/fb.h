#ifndef __FB_H__
#define __FB_H__

#include <multiboot.h>
#include <stdint.h>
#include <sys.h>
#include <glyph.h>
#include <str.h>


// #define GET_BIT(value, bit) (value & (1 << bit )) >> bit
typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
} pixel_t;

void init_framebuffer(void * address, int width, int height);
void framebuffer_put_glyph(const unsigned short symbol, int x, int y, pixel_t bg, pixel_t fg);
void framebuffer_put_block(int width, int height);
int framebuffer_raw_write(size_t start, void * src, size_t count);
void init_fb_console(int cols, int rows);
void fb_set_console_color(pixel_t fg, pixel_t bg);
void fb_console_putchar(char c);
void fb_console_write(void * src, uint32_t size, uint32_t nmemb);
void fb_console_put(char * s);
void fb_console_printf(const char * fmt, ...);

void fb_console_blink_cursor();

#endif