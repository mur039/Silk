#ifndef __UART_H_
#define __UART_H_
#include <stdint.h>
#include <isr.h>
//i have only COM1 tho
int init_uart_port(int port);
int is_transmit_empty(int port);
void uart_print(int port, const char * source, ...);
void uart_write(int port, const void * src, uint32_t size, uint32_t nmemb);
int serial_received(int port);
char serial_read(int port);
#endif