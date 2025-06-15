#ifndef __VT_H__
#define __VT_H__
#include <sys.h>
#include <tty.h>


#define VT_AUTO     1 // 	auto vt switching
#define VT_PROCESS  2 // 	process controls switching
#define VT_ACKACQ   3 // 	acknowledge switch
struct vt_mode {

    char  mode;    /* vt mode */
    char  waitv;   /* if set, hang on writes if not active */
    short relsig;  /* signal to raise on release op */
    short acqsig;  /* signal to raise on acquisition */
    short frsig;   /* unused (set to 0) */
};


//global vt state
struct vt_stat {

    unsigned short v_active;  /* active vt */
    unsigned short v_signal;  /* signal to send */
    unsigned short v_state;   /* vt bit mask */
};
    

struct vt {
    
    uint8_t attribute;
    int kmode; //  a bit map bit0, text/graphical bit1, cursor enabled, bit2: cursır visibility
    struct tty tty;
    size_t cursor_pos[2];
    struct vt_mode vtmode;

    //maybe implement terminal backing area?
    uint8_t* textbuff;
    uint8_t* attrbuff;
    char esc_encoder[16];
};


int vt_redraw();
int vt_tty_send(int scancode);
int install_virtual_terminals(int count, int row, int cols);
void vt_console_putchar( unsigned short c);
#endif