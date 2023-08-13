#ifndef _KB_H
#define _KB_H
#include <system.h>
#include <vga.h>
#include <idt.h>
#include <isrs.h>
#include <irq.h>


void keyboard_handler(struct regs *r);
#endif