#ifndef __STRING_H__
#define __STRING_H__


#include <stddef.h>

size_t strlen(const char * str);
int strcmp ( const char * str1, const char * str2 );
void* memset(void * ptr, int value, size_t num);
int memcmp( const void * ptr1, const void * ptr2, size_t num );
void * memcpy ( void * destination, const void * source, size_t num );


#endif