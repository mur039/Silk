#ifndef __STRING_H_
#define __STRING_H_
#include <stdint.h>
#include <stdarg.h>
int sprintf(char * dest, const char *format, ...);
void * memset(void* s, int c, uint32_t n); //set n byte with constant c in s
#endif