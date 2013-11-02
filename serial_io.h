#ifndef __SERIAL_IO_H__
#define __SERIAL_IO_H__

char getch_base();
void putch_base(char ch);
/* USART read/write functions structure */
typedef struct {
    char (*getch) (); //If declare as getc will cause naming conflict
    void (*putch) (char ch); //If declare as putc will cause naming conflict
} serial_ops;
/* serial msg structure */
typedef struct{
	char ch;
} serial_msg;
/*base serial io function*/
void send_byte(char ch);
char receive_byte();


#endif
