#ifndef _TIMER_H
#define _TIMER_H
#include <idt.h>
#include <isrs.h>
#include <irq.h>
#include <vga.h>
void timer_phase(int hz);
void timer_install();
void timer_wait(unsigned int ticks);

#endif