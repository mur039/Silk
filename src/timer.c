#include <sys.h>
#include <timer.h>
#include <fb.h>
/* This will keep track of how many ticks that the system
*  has been running for */
unsigned timer_ticks = 0;

void (* timer_callback)(struct regs *r) = NULL;

void timer_handler(struct regs *r)
{
    if(r->int_no != 32 + 0){return;}
    /* Increment our 'tick count' */
    timer_ticks++;

    if (timer_ticks % 10 == 0){
        // uart_write(0x3f8, "[*]One second has passes\r\n", 1, 27);
        if(timer_callback != NULL) timer_callback(r);
        
    }
    else if(timer_ticks % 40 == 0){
        // fb_console_blink_cursor();
    }
}

void register_timer_callback( void (* func)(struct regs *r) ){
    timer_callback = func;
    return;
}
/* Sets up the system clock by installing the timer handler
*  into IRQ0 */
void timer_install()
{
    /* Installs 'timer_handler' to IRQ0 */
    irq_install_handler(0, timer_handler);
}

void timer_phase(int hz)
{
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outb(0x43, 0x34); //0x36               /* Set our command byte 0x36 */
    outb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outb(0x40, divisor >> 8);     /* Set high byte of divisor */
};

void timer_wait(unsigned int ticks)
{
    unsigned long eticks;

    eticks = timer_ticks + ticks;
    while(timer_ticks < eticks);
};