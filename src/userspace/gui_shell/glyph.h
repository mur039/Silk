#ifndef __USERSPACE_GSH_GLYPH_H__
#define __USERSPACE_GSH_GLYPH_H__

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>



#define PSF1_MODE512    0x01     // 	If this bit is set, the font face will have 512 glyphs. If it is unset, then the font face will have just 256 glyphs.
#define PSF1_MODEHASTAB 0x02    // 	If this bit is set, the font face will have a unicode table.
#define PSF1_MODESEQ    0x04   // 	Equivalent to PSF1_MODEHASTAB
#define PSF1_FONT_MAGIC 0x0436
typedef struct {
    uint16_t magic; // Magic bytes for identification.
    uint8_t fontMode; // PSF font mode.
    uint8_t characterSize; // PSF character size.
} PSF1_Header;


#define PSF2_FONT_MAGIC 0x864ab572
typedef struct {
    uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
} PSF_font;

int32_t glyph_load_font(const char* font_path);
uint32_t get_glyph_size();
uint8_t * get_glyph_bitmap(const short c);


typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
} pixel_t;

void glyph_put_in(const uint8_t* framebuf, uint32_t frame_width, unsigned short symbol, int x, int y, pixel_t bg, pixel_t fg);



#endif