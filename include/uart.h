#ifndef __UART_H__
#define __UART_H__

#include <stdint-gcc.h>
#include <isr.h>
#include <circular_buffer.h>

// #include <dev.h>
// #include <filesystems/vfs.h>




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

#include <dev.h>

device_t* create_uart_device(int port);



uint32_t uart_console_write(fs_node_t * node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t uart_console_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
void uart_console_open(fs_node_t* node, uint8_t read, uint8_t write);
void uart_console_close(fs_node_t* node);


#define error(msg) uart_print(COM1, "%s->%u->%s: %s\r\n", __FILE__, __LINE__, __func__, msg);

// error(__FILE__, __LINE__, __func__, ...);

#endif