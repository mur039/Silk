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
extern int alt_gr_flag;

#define KBD_CTRL_FLAG 1 << 0
#define KBD_SHIFT_FLAG 1 << 1
#define KBD_ALT_GR_FLAG 1 << 2
extern int key_modifiers;

extern uint8_t kbd_scancode;

extern uint32_t currently_pressed_keys[4];//bit encoded
extern unsigned char kbdus[128] ;
void keyboard_handler(struct regs *r);
#endif