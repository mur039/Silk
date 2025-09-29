#include <sys.h>
#include <timer.h>
#include <fb.h>
#include <process.h>
#include <cmos.h>
#include <syscalls.h>

/* This will keep track of how many ticks that the system
*  has been running for */


uint32_t boot_epoch = 0;
uint32_t timer_ticks = 0;
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

        timer_event_t *timer = &timer_events[i];
        if (!timer->active) continue;
        
        if (timer_ticks >= timer->next_tick) {

            //wakeup the wait_queue;
            process_wakeup_list(&timer->wait_queue);

            // Fire the callback
            //well if no user data is provided, then give them the interrupt frame
            if(timer->callback){
                timer->callback( timer->user_data ? timer->user_data : r);
            }
            
            
            if (timer->interval > 0) {
                // Reschedule for periodic timer
                timer->next_tick += timer->interval;
            } else {
                // Disable one-shot timer
                timer->active = 0;
            }
        }
    }
    
    if(timer_ticks > 0 && (timer_ticks % 10) == 0 ) {
        schedule(r);
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
    int y, m, d, h, min, s;
    read_rtc(&y, &m, &d, &h, &min, &s);
    boot_epoch = to_unix_timestamp(y, m, d, h, min, s);
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
    while(timer_ticks < eticks){
        asm volatile("mov $24, %eax\r\n");
        asm volatile("int $0x80\r\n");
    }
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
        if (!timer_events[i].callback && timer_events[i].active == 0) {
            timer_events[i].next_tick = timer_ticks + delay_ticks;
            timer_events[i].interval = interval_ticks;
            timer_events[i].callback = cb;
            timer_events[i].user_data = arg;
            timer_events[i].wait_queue = list_create();
            timer_events[i].active = 1;
            return i;
        }
    }
    return -1; // No space
}

EXPORT_SYMBOL(timer_register);

timer_event_t* timer_get_by_index(unsigned int index){
    if(index > MAX_TIMERS)
        return NULL;

    return &timer_events[index];
}

int timer_destroy_timer( timer_event_t* t){
    //check if timer is infact in t
    size_t index = (size_t)(t - timer_events);
    
    //duh
    if(index > MAX_TIMERS){
        return -1;
    }

    memset(t, 0, sizeof(timer_event_t));
    return 0;
}

int timer_is_pending(timer_event_t* t){

    return timer_ticks < t->next_tick;
}

void syscall_time(struct regs* r){
    uint32_t* tloc = (void*)r->ebx;

    //ticks has resolution of ms
    uint32_t val  = boot_epoch + (timer_ticks / 1000);
    r->eax = val;

    if(tloc){ //non-NULL
        if(is_virtaddr_mapped(tloc)){
            *tloc = val;
        }
        else{
            r->eax = -EFAULT;
            return;
        }
    }


    return;
}