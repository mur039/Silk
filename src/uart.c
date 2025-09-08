#include <uart.h>
#include <sys.h>
#include <str.h>
#include <queue.h>
#include <tty.h>


tty_t serial_tty[1];

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
        if(byte == '\n'){
            outb(port, '\r'); //line dicipline mf
        }
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
}

#include <process.h>

void uart_handler(struct regs *r){
    (void)r;
    
    char c = serial_read(0x3f8);

    //with the assumption of coming from COM1
    int port_index = 0;
    struct tty* tty = &serial_tty[port_index];

    tty_ld_write(tty, &c, 1);
    
    if(c == '\r'){ //hacky
        uart_write(COM1, "\n", 1, 1);
    }
    return;
}



static int port_addr_index(int port){
    if(port == COM1){
        return 0;
    }
    return -1;
}


void tty_driver_8250_putchar(struct tty* tty, char c){
    
    int port = tty->index == 0 ? COM1 : -1;
    if(port < -1){
        return;
    }

    if(tty->termio.c_oflag & OPOST){
        if(c == '\n' && (tty->termio.c_oflag & ONLCR)){
            uart_write(port, "\r", 1, 1);
        }
    }

    if(c == '\b' || c == 127){
        uart_write(port, "\b ", 1, 3);    
    }
    
    uart_write(port, &c, 1, 1);
}

void tty_driver_8250_write(struct tty* tty, const char* buf, size_t len){
    
    int port = tty->index == 0 ? COM1 : -1;
    if(port < -1){
        return;
    }

    for(size_t i = 0; i < len; ++i){
        tty->driver->put_char(tty, buf[i]);
    }
}


struct tty_driver serial_driver = {
    .name = "8250uart",
    .num = 1,
    .write = &tty_driver_8250_write,
    .put_char = &tty_driver_8250_putchar
};

device_t* create_uart_device(int port){
    
    if( init_uart_port(port) ){ //faulty 
        return NULL;
    }

    
    int pindex = port_addr_index(port);
    if(pindex < 0){
        return NULL;
    }

    device_t* dev = kcalloc(1, sizeof(device_t));
    if(!dev)
        error("failed allocate for dev");

    dev->name = strdup("ttySxxx");
    if(!dev->name)
        error("failed allocate for dev->name");

    sprintf(dev->name, "ttyS%u", pindex);

    tty_t* tty = &serial_tty[pindex];
    memset(tty, 0, sizeof(struct tty));
    dev->priv = tty;

    tty->size.ws_col = 80;
    tty->size.ws_row = 25;
    tty->index = pindex;

    tty->termio.c_lflag = ISIG | ICANON | ECHO;
    tty->termio.c_iflag = ICRNL;
    tty->termio.c_oflag = OPOST | ONLCR;
    tty->read_wait_queue = list_create();
    tty->read_buffer = circular_buffer_create(4096);
    tty->write_buffer = circular_buffer_create(4096);
    tty->linebuffer = circular_buffer_create(512);
    tty->driver = &serial_driver;

    struct fs_ops uops = {
                            .open = (open_type_t)tty_open, 
                            .close = (close_type_t)tty_close, 
                            .write = (write_type_t)tty_write,
                            .read = (read_type_t)tty_read, 
                            .ioctl = (ioctl_type_t)tty_ioctl
                        };
    dev->ops = uops;
    
    dev->dev_type = DEVICE_CHAR;
    dev->unique_id = pindex;
      
    dev_register(dev);
    return dev;
}

