#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint-gcc.h>


int sprintf ( char * str, const char * format, ... );
int vsprintf ( char * str, const char * format, va_list arg );

int printf (const char * format, ... );
int vprintf (const char * format,  va_list arg );

extern int FILENO_STDOUT;
extern int FILENO_STDIN;
extern int FILENO_STDERR;

#endif