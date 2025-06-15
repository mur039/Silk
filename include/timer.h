#ifndef _TIMER_H__
#define _TIMER_H__
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <uart.h>
#include <pmm.h>
#include <vmm.h>

void timer_phase(int hz);
void timer_install();
void timer_wait(unsigned int ticks);
uint64_t get_ticks();
void register_timer_callback( void (* func)(struct regs *r));


#define TIMER_FREQUENCY 1000

#define MAX_TIMERS 32  // Adjustable, depending on memory limits

typedef void (*timer_func_t)(void *user_data);

typedef struct {
    uint64_t next_tick;      // Absolute tick when to call
    uint64_t interval;       // 0 for one-shot, >0 for repeating interval
    timer_func_t callback;   // Function to call, if null then that timer is disabled
    void *user_data;         // Optional argument
} timer_event_t;

int timer_register(uint64_t delay_ticks, uint64_t interval_ticks, timer_func_t cb, void *arg);

#endif