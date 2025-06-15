#ifndef __GLYPH_H__
#define __GLYPH_H__

#include <multiboot.h>
#include <stdint.h>
#include <sys.h>
#include <uart.h>
//psf has two different versions.


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
#define PSF2_HAS_UNICODE_TABLE 0x1

/* UTF8 separators */
#define PSF2_SEPARATOR  0xFF
#define PSF2_STARTSEQ   0xFE

typedef struct {
    uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
} PSF_Header;

extern const  unsigned char terminus14_psf[];
extern const unsigned int terminus14_psf_len;

extern const unsigned char consolefont_14_psf[];
extern const unsigned int consolefont_14_psf_len;

extern const unsigned char ter_u16n_psf[];
extern const unsigned int ter_u16n_psf_len;


extern const unsigned int Tamsyn8x16r_psf_len;
extern const unsigned char Tamsyn8x16r_psf[];

void parse_psf(const void * address, size_t len);
uint8_t * get_glyph_bitmap(const short c);
int unicode_to_glpyh(uint32_t unicode);    
uint32_t get_glyph_size();
uint32_t get_glyph_count();
int glyph_get_tables(int *version, void **unicode_t, void **glyph_t);




#endif