#include <uart.h>
#include <sys.h>

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
   return 0;



	// outb(port+3, 0x80);	/* set DLAB of line control reg */
	// outb(port,  0x00);	/* LS of divisor (0 -> 115200 bps */
	// outb(port+1, 0x00);	/* MS of divisor */
	// outb(port+3, 0x03);	/* reset DLAB */
	// outb(port+4, 0x0b);	/* set DTR,RTS, OUT_2 */
	// outb(port+1, 0xFF);	/* enable all intrs but writes */
	// (void)inb(port);	/* read data port to reset things (?) */
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

void uart_handler(struct regs *r){
    return;
}
