#include <system.h>
#include <uart.h>
#include <isrs.h>
#include <vga.h>
static enum COM_PORT _PORT;
static const uint16_t  uart_ports[] = {
                    [COM1] = 0x3F8,
                    [COM2] = 0x2F8,
                    [COM3] = 0x3E8,
                    [COM4] = 0x2E8,
                    [COM5] = 0x5F8,
                    [COM6] = 0x4F8,
                    [COM7] = 0x5E8,
                    [COM8] = 0x4E8 
                    };


int serialInit(enum COM_PORT port, int baudrate){

    //although no interrupts are permitted... but anyway
    outportw(uart_ports[port] + InterruptEn, 0x00);

    //DLAB = 1;
    int tmp = inportb(uart_ports[port] + LineCtrl) | 0x80; //0b10000000;
    outportb(uart_ports[port] + LineCtrl, tmp);

    //setting baudrate
    int divisor = (115200 / baudrate) <= 0 ? 1 : (int)(115200 / baudrate);
    outportb(uart_ports[port] + DataReg, divisor & 0xFF); //lower byte
    outportb(uart_ports[port] + InterruptEn, divisor >> 8); //upper byte

    //DLAB = 0; ve line control
    outportb(uart_ports[port] + LineCtrl, 0x03);// 0b00000011 // 8-bit data 1 stop bit no parity
    //enable FIFO clear,transmit; 14-byte
    outportb(uart_ports[port] + IntIdentFIFOReg, 0xc7);

    //enable interrupts
    outportb(uart_ports[port] + InterruptEn, 0x0B);
    //putting into loopback mode and sending data out test
    outportb(uart_ports[port] + ModemCtrl, 0x1E);    // Set in loopback mode, test the serial chip
    outportb(uart_ports[port] + DataReg, 0xAE);
    if(inportb(uart_ports[port] + DataReg) != 0xAE){return -1;}; //failure

    //enabling IRQ and int. #IRQ4 for COM1/3
    //outportb(uart_ports[port] + InterruptEn, 0xF);//?
    outportb(uart_ports[port] + ModemCtrl, 0x0F);
    _PORT = port;
    return 0;
}


static int is_transmit_empty() {
   return inportb(uart_ports[_PORT] + 5) & 0x20;
}
 
void write_serial(char a) {
   while (is_transmit_empty() == 0);
   outportb(uart_ports[_PORT],a);
}

int serial_received() {
   return inportb(uart_ports[_PORT] + 5) & 1;
}

char read_serial() {
   while (serial_received() == 0);
 
   return inportb(uart_ports[_PORT]);
}

void uart_handler(struct regs *r){
   if(r->int_no != 32 + 4){ //dead code
      return;
   }
   unsigned char uart_registers[8];   
   uart_registers[IntIdentFIFOReg] = inportb(uart_ports[_PORT] + IntIdentFIFOReg);
   

   switch((uart_registers[IntIdentFIFOReg] & 0xF ) | 0){
      case 0b0001: //MODEM STATUS REG
               puts("Modem Status Int.\n");
               uart_registers[ModemStat] = inportb(uart_ports[_PORT] + ModemStat);
               break;
      case 0b0011: //Transmitter holding register empty 
               puts("Transmitter holding register empty.\n");
               uart_registers[IntIdentFIFOReg] = inportb(uart_ports[_PORT] + IntIdentFIFOReg);
               //or writing to THR
               break;
      case 0b0101: //Received Data Available
               puts("Received Data Available.\n");
               write_serial(read_serial());
               uart_registers[DataReg] = inportb(uart_ports[_PORT] + DataReg);
               break;
      case 0b0111: //Receiver Line Status
               puts("Receiver Line Status.\n");
               uart_registers[LineStat] = inportb(uart_ports[_PORT] + LineStat);
               break;
      case 0b1101: //Time-out interrupt pending 
               puts("Time-out interrupt pending.\n");
               uart_registers[DataReg] = inportb(uart_ports[_PORT] + DataReg);
               break;
      default:break;
   }
   
   



   

	return;
}

