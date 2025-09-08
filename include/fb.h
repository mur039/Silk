#ifndef __FB_H__
#define __FB_H__

#include <multiboot.h>
#include <stdint.h>
#include <sys.h>
#include <glyph.h>
#include <str.h>
#include <filesystems/vfs.h>

// #define GET_BIT(value, bit) (value & (1 << bit )) >> bit
#define FRAMEBUFFER_VADDR (void*)0xFD000000 
typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
} pixel_t;

void init_framebuffer(void * address, int width, int height, int type);
void framebuffer_put_glyph(const unsigned short symbol, int x, int y, pixel_t bg, pixel_t fg);
void framebuffer_put_block(int width, int height);
uint8_t framebuffer_write_wrapper(uint8_t * buffer, uint32_t offset, uint32_t len, void * dev);
int framebuffer_raw_write(size_t start, void * src, size_t count);
uint32_t init_fb_console(int cols, int rows);
void fb_set_console_color(pixel_t fg, pixel_t bg);

void fb_console_putchar(unsigned short c);
void fb_console_write(void * src, uint32_t size, uint32_t nmemb);
void fb_console_put(char * s);
void fb_console_printf(const char * fmt, ...);
int fb_console_get_cursor();

void fb_console_blink_cursor();
uint8_t * get_framebuffer_address(void);


uint32_t console_write(fs_node_t * node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t console_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
uint32_t fb_write(fs_node_t * node, uint32_t offset, uint32_t size, uint8_t* buffer);
void fb_console_va_printf(const char* fmt, va_list args);

void pci_disp_irq_handler(struct regs *r);
void install_basic_framebuffer(uint32_t* base, uint32_t width, uint32_t height, uint32_t bpp);

#endif