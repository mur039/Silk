#include <uart.h>
#include <sys.h>
#include <str.h>

int init_uart_port(int port)
{
    //gotta do somthing about it , interrupts doesn't trigger
    outb(port + 1, 0x00);    // Disable all interrupts
    outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00);    //                  (hi byte)
    outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(port + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(port + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
 
   // Check if serial is faulty (i.e: not same byte as sent)
   if(inb(port + 0) != 0xAE) {
      return 1;
   }
 
   // If serial is not faulty set it in normal operation mode
   // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
   outb(port + 4, 0x0F);
   outb(port + 1, 0b0001);//wnable daata available interrupt
   return 0;
}


int is_transmit_empty(int port){
    return inb(port + 5) & 0x20;
}
void uart_write(int port, const void * src, uint32_t size, uint32_t nmemb){
    while(is_transmit_empty(port) == 0);
    for(uint32_t i = 0; i < nmemb; ++i){
        uint8_t byte = 0;
        byte = *((uint8_t *)src + (i*size));
        outb(port, byte);
    }
}

int serial_received(int port){
   return inb(port + 5) & 1;
}
char serial_read(int port){
    while (serial_received(port) == 0);
    return inb(port);
}

static int dest_port = 0;
static char uart_gprintf_wrapper(char c){
    uart_write(dest_port, &c, 1, 1);
    return c;
}

void uart_print(int port, const char * source, ...){
    va_list arg;
    va_start(arg, source);
    dest_port = port;
    va_gprintf(
        uart_gprintf_wrapper,
        source,
        arg
    );
    // for(int i = 0; source[i] != '\0'; ++i){
    //     uart_write(port, &source[i], 1, 1);
    // }
    return;
}


extern volatile int is_received_char;
extern volatile char ch;
void uart_handler(struct regs *r){
    (void)r;
    char c = serial_read(0x3f8);


    is_received_char = 1;
    ch = c;

    // semaphore_uart_handler->value++;
    // pcb_t * proc = queue_dequeue_item(&semaphore_uart_handler->queue);
    // if(proc){
    //     proc->state = TASK_RUNNING;
        
    // }
    // fb_console_printf("%c:%x\n",ch, ch);
    return;
}




