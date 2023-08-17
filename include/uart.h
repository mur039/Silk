
#ifndef _UART_H_
#define _UART_H_
#include <stdint.h>
#include <isrs.h>
enum COM_PORT{
    COM1 = 0,
    COM2,
    COM3,
    COM4,
    COM5,
    COM6,
    COM7,
    COM8
} ;

enum UART_REGISTERS{
    DataReg = 0, //DLAB = 0 // DLAB = 1 least sig. byte of baudrate divisor
    InterruptEn, //DLAB = 0 // DLAB = 1 most sig. byte of baudrate divisor
    IntIdentFIFOReg,
    LineCtrl, // (LineCtrl) & (1<7)(0x80) => DLAB
    ModemCtrl,
    LineStat,
    ModemStat,
    ScratchReg
} ;
/*
+0  0   Data register. Reading this registers read from the Receive buffer. Writing to this register writes to the Transmit buffer.
+1 	0 	Interrupt Enable Register.
+0 	1 	With DLAB set to 1, this is the least significant byte of the divisor value for setting the baud rate.
+1 	1 	With DLAB set to 1, this is the most significant byte of the divisor value.
+2 	- 	Interrupt Identification and FIFO control registers
+3 	- 	Line Control Register. The most significant bit of this register is the DLAB.
+4 	- 	Modem Control Register.
+5 	- 	Line Status Register.
+6 	- 	Modem Status Register.
+7 	- 	Scratch Register. */
int serialInit(enum COM_PORT port, int baudrate);
void write_serial(char a);
char read_serial();
int serial_received();
void uart_handler(struct regs *r);
#endif
