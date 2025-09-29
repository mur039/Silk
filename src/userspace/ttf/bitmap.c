#include "bitmap.h"
#include <string.h>

int surface_copy_surface(struct surface *dest, struct surface *src, size_t dx, size_t dy){

    if(dest->s_bpp == 1){}
    if(dest->s_bpp == 8){}
    else if(dest->s_bpp == 16){}
    else if(dest->s_bpp == 32){

        //well copy src to dest
        //check if src exceeds the dest
        int  _srcwidth = src->s_width;
        int  _srcheigth = src->s_height;

        if( (dx + _srcwidth) > dest->s_width  ){
            _srcwidth = dest->s_width - dx;
        }

        if( (dy + _srcheigth) > dest->s_height  ){
            _srcheigth = dest->s_height - dy;
        }

        unsigned long* dptr = dest->s_back;

        for(int y = 0; y < _srcheigth; ++y){
            for(int x = 0; x < _srcwidth; ++x){
                
                unsigned char* srcgrey = src->s_back;
                unsigned long val = (srcgrey[x + (y*src->s_width)] / 255.0) * 0xffffff;
                dptr[dx + x + ((dy + y)* dest->s_width)  ] = val;
            }
        }

    }

    return 0;
}


int fbcon_clear(struct fbcon* con){
    struct surface* s = con->fb_surface;
    memset(s->s_back, 0, s->s_height * s->s_pitch);

    con->pos_c = 0;
    con->pos_r = 0;
    return 0;
}