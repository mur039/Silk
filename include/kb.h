#ifndef _KB_H
#define _KB_H
#include <sys.h>
// #include <vga.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>


void keyboard_handler(struct regs *r);
#endif