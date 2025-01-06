#ifndef __UART_H_
#define __UART_H_
#include <stdint.h>
#include <isr.h>

#define COM1 0x3f8
#define UART_IRQ 4
//i have only COM1 tho
int init_uart_port(int port);
int is_transmit_empty(int port);
void uart_print(int port, const char * source, ...);
void uart_write(int port, const void * src, uint32_t size, uint32_t nmemb);
int serial_received(int port);
char serial_read(int port);
void uart_handler(struct regs *r);
void* uart_read(int port, unsigned char *buffer);

#endif