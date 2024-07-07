#ifndef _KB_H
#define _KB_H
#include <sys.h>
// #include <vga.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>


extern unsigned char kbdus[128] ;
void keyboard_handler(struct regs *r);
#endif