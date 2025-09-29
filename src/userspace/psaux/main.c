#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <termios.h>



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
    unsigned long raw; //struct itself is only 3-byte
} psaux_pack_t;


#define PSAUX_PACKET_MASK 0xFFFFFF
#define PSAUX_BUTTON_MASK 0x7

#define PSAUX_LEFT_BUTTON_MASK   0x1
#define PSAUX_MIDDLE_BUTTON_MASK 0x2
#define PSAUX_RIGHT_BUTTON_MASK  0x4


typedef struct {
    int x;
    int y;
    uint8_t buttons; // bits 0=Left, 1=Right, 2=Middle
} mouse_state_t;

// Convert raw PS/2 packet to signed movement
void parse_ps2_packet(uint8_t packet[3], mouse_state_t *mouse) {
    uint8_t b0 = packet[0];
    uint8_t b1 = packet[1];
    uint8_t b2 = packet[2];

    // Extract buttons
    mouse->buttons = b0 & 0x07;

    // Check overflow flags (bits 6 and 7)
    int x_over = (b0 & 0x40) != 0;
    int y_over = (b0 & 0x80) != 0;

    // Calculate signed X movement
    int8_t dx = (b0 & 0x10) ? (int8_t)(b1 | 0xFFFFFF00) : (int8_t)b1;

    // Calculate signed Y movement (invert for screen coordinates)
    int8_t dy = (b0 & 0x20) ? (int8_t)(b2 | 0xFFFFFF00) : (int8_t)b2;
    if(dy > 0){
        dy *= 1;
    }
    else{
        dy /= 4;
    }


    dy = -dy;

    // Optionally clamp or ignore if overflow
    if(x_over) dx = (dx > 0) ? 127 : -128;
    if(y_over) dy = (dy > 0) ? 127 : -128;

    mouse->x += dx;
    mouse->y += dy;
}


int main(void){

    int psauxfd = open("/dev/psaux", O_RDONLY);
    if(psauxfd < 0){
        puts("Failed  to open device file\n");
        exit(EXIT_FAILURE);
    }

    psaux_pack_t data;
    int err;
    
    int mouse_x = 0, mouse_y = 0;
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

    
    mouse_state_t m = {.x = 0, .y = 0};
    while(1){

        signed char packet[3];
        err = read(psauxfd, packet, 3);
        if(err < 0 ){
            puts("Failed to read\n");
            exit(EXIT_FAILURE);
        }

        parse_ps2_packet(packet, &m);
        
        if(m.x < 0) m.x = 0;
        if(m.x > 1000) m.x = 1000;
        
        if(m.y < 0) m.y = 0;
        if(m.y > 1000) m.y = 1000;

        int col, row;
        col = m.x * size.ws_col / 1000;
        row = m.y * size.ws_row / 1000;
        
        
        // printf("(col, row) %u %u\n", col, row);

        char buffer[16];
        sprintf(buffer, "\033[%u;%uH", row , col);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        
    }

    return 0;
}