#include <sys.h>
#include <timer.h>
#include <fb.h>
/* This will keep track of how many ticks that the system
*  has been running for */


uint64_t timer_ticks = 0;
uint32_t overflow_ticks = 0;

timer_event_t timer_events[MAX_TIMERS];
void (* timer_callback)(struct regs *r) = NULL;

void timer_handler(struct regs *r)
{
    
    if(r->int_no != 32 + 0){return;}

    //catch overflowss
    if( (timer_ticks + 1) == 0){
        overflow_ticks++;
    }
    timer_ticks++;


    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timer_events[i].callback) continue;
        
        if (timer_ticks >= timer_events[i].next_tick) {
            // Fire the callback
            //well if no user data is provided, then give them the interrupt frame
            if(timer_events[i].user_data)
                timer_events[i].callback(timer_events[i].user_data);
            else{
                timer_events[i].callback(r);
            }

            if (timer_events[i].interval > 0) {
                // Reschedule for periodic timer
                timer_events[i].next_tick += timer_events[i].interval;
            } else {
                // Disable one-shot timer
                memset(&timer_events[i], 0, sizeof(timer_event_t));
            }
        }
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

uint64_t get_ticks(){

    uint64_t total_tick  = overflow_ticks;
    total_tick << 32;
    total_tick |= timer_ticks;
    return total_tick;
}

int timer_register(uint64_t delay_ticks, uint64_t interval_ticks, timer_func_t cb, void *arg)
{
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timer_events[i].callback) {
            timer_events[i].next_tick = timer_ticks + delay_ticks;
            timer_events[i].interval = interval_ticks;
            timer_events[i].callback = cb;
            timer_events[i].user_data = arg;
            return 1;
        }
    }
    return 0; // No space
}