#include "glyph.h"
#include <stdint-gcc.h>


typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;

typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;


static uint32_t glyph_height;
static uint8_t* font_addr = NULL;
static uint8_t* glyph_addr = NULL;


int32_t glyph_load_font(const char* font_path){

    int fontfd = open("/share/screenfonts/consolefont.psf", O_RDONLY);
    int font_Size = lseek(fontfd, 0, SEEK_END);

    font_addr = malloc(font_Size);
    if(!font_addr){
        puts("malloc failed in fb_font_addr");
        return -1;
    }

    lseek(fontfd, 0, SEEK_SET);
    read(fontfd, font_addr, font_Size);

    //now validation of the font
    uint16_t * magic_1 = font_addr;
    if(*magic_1 == PSF1_FONT_MAGIC){
        
        glyph_height = ((PSF1_Header*)font_addr)->characterSize;
        glyph_addr = ((uint8_t*)font_addr) + sizeof(PSF1_Header);
        return 1;
    }
    else if(*(uint32_t*)magic_1 == PSF_FONT_MAGIC){
        return 2;
    }
    else{
        puts("Invalid magic number.\n");
        free(font_addr);
        return -1;
    }

    

}


uint32_t get_glyph_size(){
    return glyph_height;
}

uint8_t * get_glyph_bitmap(const short c){
    uint8_t * glyph = glyph_addr;
    return &glyph[c * glyph_height];
}


#define GET_BIT(value, bit) ((value >> bit) & 1)

void glyph_put_in(const uint8_t* framebuf, uint32_t frame_width, unsigned short symbol, int x, int y, pixel_t bg, pixel_t fg){
     uint8_t * glyph = get_glyph_bitmap(symbol);


     for (size_t j = 0; j < get_glyph_size(); j++)
    {
        pixel_t * row = (pixel_t *)&framebuf[(y + j) * 4 * frame_width];
        for(int i = 0; i < 8; ++i){ //bits
            row[x + i] = 
                        GET_BIT(glyph[j], (7 -i)) ? fg : bg;
                
        }
    }
}