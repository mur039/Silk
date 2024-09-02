#ifndef __PS2_MOUSE_H__
#define __PS2_MOUSE_H__

#include <ps2.h>

#define PS2_MOUSE_IRQ 12

/*COMMANDS*/
#define PS2_MOUSE_ENABLE_REPORTING 0xF4
#define PS2_MOUSE_DISABLE_REPORTING 0xF5
#define PS2_MOUSE_DEFAULTS 0xF6


typedef union 
{
    struct 
    {
        union status{
            struct
            {
                char y0:1;
                char x0:1;
                char ys:1;
                char xs:1;
                char one:1;
                char bm:1;
                char br:1;
                char bl:1;    
            };
            char raw;
        } bits;
        signed char x_axis_move;
        signed char y_axis_move;
    };
    u32 raw; //struct itself is only 3-byte
} ps2_mouse_generic_package_t;


void ps2_mouse_initialize();

void ps2_mouse_send_command(unsigned char command);
void ps2_mouse_handler(struct regs *r);
int32_t ps2_mouse_read();
uint8_t ps2_mouse_fs_read(uint8_t * buffer, uint32_t offset, uint32_t len, void * dev);



#endif