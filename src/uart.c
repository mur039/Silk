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





/*

int initialize_serial_port(SerialDevice * device){
    u16 divisor = 115200 / device->SerialBaudrate;
    outb(device->BasePortAddress + 1, 0x00);    // Disable all interrupts
    outb(device->BasePortAddress + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(device->BasePortAddress + 0, divisor & 0xff);    // Set divisor to 3 (lo byte) 38400 baud
    outb(device->BasePortAddress + 1, divisor >>   8);    //                  (hi byte)
    outb(device->BasePortAddress + 3, device->Registers.LineControl_DLAB);// 8 bits, no parity, one stop bit
    outb(device->BasePortAddress + 2, device->Registers.InterruptIdentification_fifoControl);    // Enable FIFO, clear them, with 14-byte threshold
    outb(device->BasePortAddress + 4, device->Registers.ModemControl);    // IRQs enabled, RTS/DSR set
    outb(device->BasePortAddress + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(device->BasePortAddress + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
    
    // Check if serial is faulty (i.e: not same byte as sent)
    if(inb(device->BasePortAddress + 0) != 0xAE) {
       return 1;
    }
    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(device->BasePortAddress + 4, 0x0F);

    //NOW  we have working serial port
    char lineBuffer[64] = {0};
    u32 head, tail = 0;
    serial_write( *device, "\x1b[999;999H\x1b[6n", 1, 15);

    //read until character 'R'
    for(head = 0; head < 64; ++head){ //number of character read capped, so that buffere wouldn't overflow
        lineBuffer[head] =serial_read(*device);
        if(lineBuffer[head] == 'R'){ break;}
    }

    u32 length[2] = {0, 0};
    char endcond[2] = {';', 'R'};
    for(int i = 0; i < 2; ++i){

        for(; lineBuffer[tail] != endcond[i]; ++tail ){
            if(lineBuffer[tail] >= '0' && lineBuffer[tail] <= '9' ){
                length[i] *= 10;
                length[i] += lineBuffer[tail] - '0';
            }
            else{
                continue;
            }
        }

    }
    device->tty.row_size = length[0];
    device->tty.col_size = length[1];
    serial_write(*device, "\x1b[0;0H", 1, 7); //moves cursor to 0:0
    device->tty.cursor_position_row = 0;
    device->tty.cursor_position_col = 0;
    return 0; 
}

static int is_transmit_empty(SerialDevice dev){
    return inb(dev.BasePortAddress + 5) & 0x20;
}
void serial_write(SerialDevice dev, const void * src, u32 size, u32 nmemb){
    while(is_transmit_empty(dev) == 0);
    for(u32 i = 0; i < nmemb; ++i){
        u8 byte = 0;
        byte = *((u8 *)src + (i*size));
        outb(dev.BasePortAddress, byte);
    }
}

static int serial_received(SerialDevice dev){
   return inb(dev.BasePortAddress + 5) & 1;
}

u8 serial_read(SerialDevice dev){
    while (serial_received(dev) == 0);
    return inb(dev.BasePortAddress);
}

usize serial_print(SerialDevice dev, const char * source){
    for(int i = 0; source[i] != '\0'; ++i){
        serial_write(dev, &source[i], 1, 1);
    }
    return 0;
}



*/
