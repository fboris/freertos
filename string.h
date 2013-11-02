#ifndef _STRING_H_
#define _STRING_H_
#include <stddef.h>
void *memset(void *dest, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
void *memmove(void *dest, const void *src, size_t count);
char *strcpy(char *dest, const char *src);
char *strchr(const char *s, int c);
size_t strlen(const char *string);
int strcmp(const char *str_a, const char *str_b);
int strncmp(const char *str_a, const char *str_b, size_t n);
char *strcat(char *dest, const char *src);
char *strdup(const char *str);
#endif
