#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1/UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-ONES & ~(x) & HIGHS)

size_t strlen(const char *s)
{
        const char *a = s;
        const size_t *w;
        for (; (uintptr_t) s % ALIGN; s++)
                if (!*s) return (s - a);
        for (w = (const void *) s; !HASZERO(*w); w++);
        for (s = (const void *) w; *s; s++);
        return (s - a);
}

void *memmove(void *dest, const void *src, size_t count)
{
	char *tmp;
	const char *s;

	if(dest <= src) {
		tmp = dest;
		s = src;
		while(count--)
			*tmp++ = *s++;
	} else {
		tmp = dest;
		tmp += count;
		s= src;
		s += count;
		while(count--)
			*--tmp = *--s;
	}
	return dest;
}                   
