#include <string.h>
#include <stdint.h>
#include <stdarg.h> 
#include <limits.h>
#include <unistd.h>
#include "util.h"
/*FreeRTOS relative */
#include <FreeRTOS.h>
#include <semphr.h>
/* Filesystem relative */
#include "fio.h"
#include "filesystem.h"
#include "osdebug.h"
#include "hash-djb2.h"
/*Serial IO*/
#include "serial_io.h"




extern serial_ops serial;

static struct fddef_t fio_fds[MAX_FDS];

static ssize_t stdin_read(void * opaque, void * buf, size_t count) {
    int i;
    
    char * data = (char *) buf;

    for (i = 0; i < count; i++)
        data[i] = (char)receive_byte();
    
    return count;
}

static ssize_t stdout_write(void * opaque, const void * buf, size_t count) {
    int i;
    const char * data = (const char *) buf;
    
    for (i = 0; i < count; i++)
        send_byte(data[i]);
    
    return count;
}

static xSemaphoreHandle fio_sem = NULL;

__attribute__((constructor)) void fio_init() {
    memset(fio_fds, 0, sizeof(fio_fds));
    fio_fds[0].fdread = stdin_read;
    fio_fds[1].fdwrite = stdout_write;
    fio_fds[2].fdwrite = stdout_write;
    fio_sem = xSemaphoreCreateMutex();
}

struct fddef_t * fio_getfd(int fd) {
    if ((fd < 0) || (fd >= MAX_FDS))
        return NULL;
    return fio_fds + fd;
}

static int fio_is_open_int(int fd) {
    if ((fd < 0) || (fd >= MAX_FDS))
        return 0;
    int r = !((fio_fds[fd].fdread == NULL) &&
              (fio_fds[fd].fdwrite == NULL) &&
              (fio_fds[fd].fdseek == NULL) &&
              (fio_fds[fd].fdclose == NULL) &&
              (fio_fds[fd].opaque == NULL));
    return r;
}

static int fio_findfd() {
    int i;
    
    for (i = 0; i < MAX_FDS; i++) {
        if (!fio_is_open_int(i))
            return i;
    }
    
    return -1;
}

int fio_is_open(int fd) {
    int r = 0;
    xSemaphoreTake(fio_sem, portMAX_DELAY);
    r = fio_is_open_int(fd);
    xSemaphoreGive(fio_sem);
    return r;
}

int fio_open(fdread_t fdread, fdwrite_t fdwrite, fdseek_t fdseek, fdclose_t fdclose, void * opaque) {
    int fd;
//    DBGOUT("fio_open(%p, %p, %p, %p, %p)\r\n", fdread, fdwrite, fdseek, fdclose, opaque);
    xSemaphoreTake(fio_sem, portMAX_DELAY);
    fd = fio_findfd();
    
    if (fd >= 0) {
        fio_fds[fd].fdread = fdread;
        fio_fds[fd].fdwrite = fdwrite;
        fio_fds[fd].fdseek = fdseek;
        fio_fds[fd].fdclose = fdclose;
        fio_fds[fd].opaque = opaque;
    }
    xSemaphoreGive(fio_sem);
    
    return fd;
}

ssize_t fio_read(int fd, void * buf, size_t count) {
    ssize_t r = 0;
//    DBGOUT("fio_read(%i, %p, %i)\r\n", fd, buf, count);
    if (fio_is_open_int(fd)) {
        if (fio_fds[fd].fdread) {
            r = fio_fds[fd].fdread(fio_fds[fd].opaque, buf, count);
        } else {
            r = -3;
        }
    } else {
        r = -2;
    }
    return r;
}

ssize_t fio_write(int fd, const void * buf, size_t count) {
    ssize_t r = 0;
//    DBGOUT("fio_write(%i, %p, %i)\r\n", fd, buf, count);
    if (fio_is_open_int(fd)) {
        if (fio_fds[fd].fdwrite) {
            r = fio_fds[fd].fdwrite(fio_fds[fd].opaque, buf, count);
        } else {
            r = -3;
        }
    } else {
        r = -2;
    }
    return r;
}

off_t fio_seek(int fd, off_t offset, int whence) {
    off_t r = 0;
//    DBGOUT("fio_seek(%i, %i, %i)\r\n", fd, offset, whence);
    if (fio_is_open_int(fd)) {
        if (fio_fds[fd].fdseek) {
            r = fio_fds[fd].fdseek(fio_fds[fd].opaque, offset, whence);
        } else {
            r = -3;
        }
    } else {
        r = -2;
    }
    return r;
}

int fio_close(int fd) {
    int r = 0;
//    DBGOUT("fio_close(%i)\r\n", fd);
    if (fio_is_open_int(fd)) {
        if (fio_fds[fd].fdclose)
            r = fio_fds[fd].fdclose(fio_fds[fd].opaque);
        xSemaphoreTake(fio_sem, portMAX_DELAY);
        memset(fio_fds + fd, 0, sizeof(struct fddef_t));
        xSemaphoreGive(fio_sem);
    } else {
        r = -2;
    }
    return r;
}

void fio_set_opaque(int fd, void * opaque) {
    if (fio_is_open_int(fd))
        fio_fds[fd].opaque = opaque;
}

#define stdin_hash 0x0BA00421
#define stdout_hash 0x7FA08308
#define stderr_hash 0x7FA058A3

static int devfs_open(void * opaque, const char * path, int flags, int mode) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
//    DBGOUT("devfs_open(%p, \"%s\", %i, %i)\r\n", opaque, path, flags, mode);
    switch (h) {
    case stdin_hash:
        if (flags & (O_WRONLY | O_RDWR))
            return -1;
        return fio_open(stdin_read, NULL, NULL, NULL, NULL);
        break;
    case stdout_hash:
        if (flags & O_RDONLY)
            return -1;
        return fio_open(NULL, stdout_write, NULL, NULL, NULL);
        break;
    case stderr_hash:
        if (flags & O_RDONLY)
            return -1;
        return fio_open(NULL, stdout_write, NULL, NULL, NULL);
        break;
    }
    return -1;
}

void register_devfs() {
    DBGOUT("Registering devfs.\r\n");
    register_fs("dev", devfs_open, NULL);
}


int puts(const char* msg)
{   
    for(; *msg; msg++)
    serial.putch(*msg);

    return 1;
}

int sprintf(char *dst, const char *fmt, ...)
{
  union {
    int i;
    const char *s;
    unsigned u;
  } argv;
  char *p = dst;
  va_list arg_list;

  va_start(arg_list, fmt);
  for (; *fmt; ++fmt) {
    if (*fmt == '%') {
      switch (*++fmt) {
        case '%':
          *p++ = '%';
        break;
        case 'c':
          argv.i = va_arg(arg_list, int);
          *p++ = (char)argv.i;
        break;
        case 'd':
        case 'i':
          argv.i = va_arg(arg_list, int);
          itoa(argv.i, p, 10);
          p += strlen(p);
        break;
        case 'u':
          argv.u = va_arg(arg_list, unsigned);
          utoa(argv.u, p, 10);
          p += strlen(p);
        break;
        case 's':
          argv.s = va_arg(arg_list, const char *);
          strcpy(p, argv.s);
          p += strlen(p);
        break;
        case 'x':
        case 'X':
          argv.u = va_arg(arg_list, unsigned);
          utoa(argv.u, p, 16);
          p += strlen(p);
        break;
      }
    }
    else
      *p++ = *fmt;
  }
  va_end(arg_list);
  *p = '\0';

  return p - dst;
}
 
int printf(const char *fmt, ...)
{
    char buf[8];
    union {
        int i;
        const char *s;
        unsigned u;
    } argv;
    va_list arg_list;
    
    va_start(arg_list, fmt);
    for (; *fmt; ++fmt) {
        if (*fmt == '%') {
            switch (*++fmt) {
                case '%':
                    serial.putch('%');
                break;
                case 'c':
                    argv.i = va_arg(arg_list, int);
                    serial.putch(argv.i);
                break;
                case 'd':
                case 'i':
                    argv.i = va_arg(arg_list, int);
                    itoa(argv.i, buf, 10);
                    puts(buf);
                break;
                case 'u':
                    argv.u = va_arg(arg_list, unsigned);
                    utoa(argv.u, buf, 10);
                    puts(buf);
                break;
                case 's':
                    argv.s = va_arg(arg_list, const char *);
                    puts(argv.s);
                break;
                case 'x':
                case 'X':
                    argv.u = va_arg(arg_list, unsigned);
                    utoa(argv.u, buf, 16);
                    puts(buf);
                break;
            }
        }
        else {
            serial.putch(*fmt);
        }
    } 
    va_end(arg_list);
    return 1;

}