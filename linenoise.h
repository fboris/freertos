#ifndef __LINENOISE_H
#define __LINENOISE_H

typedef struct linenoiseCompletions {
  size_t len;
  char **cvec;
} linenoiseCompletions;

/* USART read/write functions structure */
typedef struct {
    char (*getch) (); //If declare as getc will cause naming conflict
    void (*putch) (char ch); //If declare as putc will cause naming conflict
    int (*puts) (const char *msg);
} serial_ops;

typedef void(linenoiseCompletionCallback)(const char *, linenoiseCompletions *);
void linenoiseSetCompletionCallback(linenoiseCompletionCallback *);
void linenoiseAddCompletion(linenoiseCompletions *, char *);

char *linenoise(const char *prompt);
int linenoiseHistoryAdd(const char *line);
void linenoiseClearScreen(void);


#endif
