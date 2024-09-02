#ifndef _KB_H
#define _KB_H
#include <sys.h>
// #include <vga.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>

#define PS2_KEYBOARD_IRQ 1

extern int ctrl_flag;
extern int shift_flag;
extern uint32_t currently_pressed_keys[4];//bit encoded
extern unsigned char kbdus[128] ;
void keyboard_handler(struct regs *r);
#endif