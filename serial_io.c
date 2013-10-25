#include "serial_io.h"
#include "fio.h"
char getch_base()
{
    char ch;
    fio_read(0, &ch, 1);
    return ch;
}

void putch_base(char ch)
{
    fio_write(1, &ch, 1);
}

/* Serial read/write callback functions */
serial_ops serial = {
    .getch = getch_base,
    .putch = putch_base,
};


