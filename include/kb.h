#ifndef _KB_H
#define _KB_H
#include <sys.h>
// #include <vga.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>

#define PS2_KEYBOARD_IRQ 1


#define KBD_CTRL_FLAG 1 << 0
#define KBD_SHIFT_FLAG 1 << 1
#define KBD_ALT_GR_FLAG 1 << 2

#define PS2_SCANCODE_EXTENDED 0xE0
#define PS2_SCANCODE_LSHIFT   0x2A
#define PS2_SCANCODE_RSHIFT   0x36
//has with extended otherway around
#define PS2_SCANCODE_LCONTROL 0x1D
#define PS2_SCANCODE_LALT     0x38
#define PS2_SCANCODE_EXTENDED 0xE0
#define PS2_SCANCODE_EXTENDED 0xE0

extern uint32_t currently_pressed_keys[4];//bit encoded
extern unsigned char kbdus[128] ;
void keyboard_handler(struct regs *r);
void ps2_kbd_handler(struct regs *r);
void ps2_kbd_initialize();

#include <process.h>

#endif