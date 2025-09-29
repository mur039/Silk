#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stddef.h>


#define SURFACE_BITMAP    1
#define SURFACE_GREYSCALE 2
#define SURFACE_ARGB      4


struct surface {
    void* s_back;
    size_t s_width;
    size_t s_height;
    size_t s_pitch;
    int s_bpp;
    int s_flags;
};


int surface_copy_surface(struct surface *dest, struct surface *src, size_t dx, size_t dy);




//fbcon
struct fbcon {
    struct surface* fb_surface;
    int col, row;
    int pos_c, pos_r;
};

int fbcon_clear(struct fbcon* con);





#endif