#ifndef __UTIL_H__
#define __UTIL_H__

void *pvPortRealloc(void *ptr, size_t len);
char *utoa(unsigned int num, char *dst, unsigned int base);
char *itoa(int num, char *dst, int base);

#endif
