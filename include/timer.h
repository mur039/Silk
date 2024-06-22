#ifndef _TIMER_H__
#define _TIMER_H__
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <uart.h>
void timer_phase(int hz);
void timer_install();
void timer_wait(unsigned int ticks);

#endif