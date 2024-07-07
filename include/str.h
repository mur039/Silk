#ifndef __STRING_H_
#define __STRING_H_
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

int va_sprintf(char * dest, const char *format, va_list args);
int sprintf(char * dest, const char *format, ...);
int gprintf(char (* write)(char c), const char *format, ...);
int va_gprintf(char (* write)(char c), const char *format, va_list args);
void * memset(void* s, int c, uint32_t n); //set n byte with constant c in s
void * memcpy(void * dest, const void * src, uint32_t n);
int memcmp(const void *s1, const void *s2, size_t n );
size_t strlen(const char * s);
#endif