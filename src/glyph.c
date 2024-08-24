#include "glyph.h"

static uint32_t version;
static uint32_t glyph_height;
static uint8_t * screen_font_addr = NULL;

void parse_psf(void * address){

    uint16_t * magic_1 = address;       
    if(*magic_1 == PSF1_FONT_MAGIC){
        uart_print(COM1, "Version PSF1 found\r\n");
        version = 1; //assume 256 only
        glyph_height = ((PSF1_Header*)address)->characterSize;
        screen_font_addr = ((uint8_t*)address) + sizeof(PSF1_Header);

    }
    else if(*(uint32_t*)magic_1 == PSF_FONT_MAGIC){
        uart_print(COM1, "Version PSF found\r\n");
    }
    else{
        uart_print(COM1, "Invalid magic number.\r\n");
    }

    uart_print(COM1, "Font mode-> %x\r\n", ((PSF1_Header*)address)->fontMode);
}

uint32_t get_glyph_size(){
    return glyph_height;
}

uint8_t * get_glyph_bitmap(const short c){
    uint8_t * glyph = screen_font_addr;
    return &glyph[c * glyph_height];
}
