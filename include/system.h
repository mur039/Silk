#ifndef _SYSTEM_H_
#define _SYSTEM_H_
#include <stdint.h>
#include <stddef.h>

unsigned char *memcpy(void *dest, const  void *src, int count);
unsigned char *memset(void *dest, unsigned char val, int count);
unsigned short *memsetw(void *dest, unsigned short val, int count);
int strlen(const char *str);
void outportb (unsigned short _port, unsigned char _data);
void outportw (unsigned short _port, unsigned short _data);
uint8_t inportb (unsigned short _port);
uint16_t inportw(unsigned short _port);

#endif