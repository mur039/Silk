#ifndef __STRING_H_
#define __STRING_H_
#include <stdint.h>
#include <stdint-gcc.h>
#include <stdarg.h>
#include <stddef.h>

int va_sprintf(char * dest, const char *format, va_list args);
int sprintf(char * dest, const char *format, ...);
int gprintf(char (* write)(char c), const char *format, ...);
int va_gprintf(char (* write)(char c), const char *format, va_list args);

void * memset(void* s, int c, uint32_t n); //set n byte with constant c in s
void * memcpy ( void * destination, const void * source, size_t num );
int memcmp( const void * ptr1, const void * ptr2, size_t num );
size_t strlen(const char * s);
int strcmp(const char *str1, const char * str2);
int strncmp(const char *str1, const char * str2, int n);
int is_char_in_str(const char c, const char * str);
#endif