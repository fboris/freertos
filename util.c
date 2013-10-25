#include <string.h> 

char *utoa(unsigned int num, char *dst, unsigned int base)
{
	char buf[33] = {0};
	char *p = &buf[32];

	if (num == 0){
    	*--p = '0';
    }
  	else{
    	for (; num; num/=base)
      	*--p = "0123456789ABCDEF" [num % base];
  	}
  	return strcpy(dst, p);
} 
char *itoa(int num, char *dst, int base)
{
 	if (base == 10 && num < 0) {
		utoa(-num, dst+1, base);
    	*dst = '-';
  	}
  	else
   		utoa(num, dst, base);

  	return dst;
} 