#ifndef __STDLIB_H__
#define __STDLIB_H__
#include <stddef.h>

#define NULL (void*)0


int abs(int n);
int atoi (const char * str);

void* malloc(size_t size);
void* calloc(size_t nmemb, size_t size);
void  free(void *ptr);
void malloc_init();


#endif