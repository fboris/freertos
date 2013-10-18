#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "linenoise.h"  

#include "main.h"

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 1024 //4096 is too much to this environment, it will crash!

static int mlmode = 0;  /* Multi line mode. Default is single line. */

/* USART read/write callback functions */
char (*serial_getc)();
void (*serial_putc)(char ch);
void (*serial_puts)(const char *str);

void linenoiseCallbackFuncInit() {
	serial_getc = receive_byte;
	serial_putc = send_byte;
	serial_puts = queue_str_task;
}

struct linenoiseState {       
    char *buf;          /* Edited line buffer. */      
    size_t buflen;      /* Edited line buffer size. */ 
    const char *prompt; /* Prompt to display. */       
    size_t plen;        /* Prompt length. */
    size_t pos;         /* Current cursor position. */ 
    size_t oldpos;      /* Previous refresh cursor position. */
    size_t len;         /* Current edited line length. */
    size_t cols;        /* Number of columns in terminal. */
    size_t maxrows;     /* Maximum num of rows used so far (multiline mode) */
    int history_index;  /* The history index we are currently editing. */                                        
};

void linenoiseClearScreen(void) {
	serial_puts("\x1b[H\x1b[2J");
}

static void refreshSingleLine(struct linenoiseState *l) {
    size_t plen = strlen(l->prompt);
    char *buf = l->buf;
    size_t len = l->len;
    size_t pos = l->pos;

    while((plen+pos) >= l->cols) {
        buf++;
        len--;
        pos--;
    }
    while (plen+len > l->cols) {
        len--;
    }

    /* Cursor to left edge */ 
    serial_puts("\x1b[0G");
    /* Write the prompt and the current buffer content */
    serial_puts(l->prompt);
    serial_puts(buf);
    /* Erase to right */
    serial_puts("\x1b[0K");
    /* Move cursor to original position. */
    char sq[] = "\x1b[0G\x1b[12C"; //the max columes of Terminal environment is 80
    /* Set the count of moving cursor */
    sq[6] = (pos+plen) / 10 + 0x30;  
    sq[7] = (pos+plen) % 10 + 0x30;
    serial_puts(sq);
}

static void refreshLine(struct linenoiseState *l) {
    if (mlmode) {
        //refreshMultiLine(l);
    } else {
        refreshSingleLine(l);
    }
}

int linenoiseEditInsert(struct linenoiseState *l, int c) {
    if (l->len < l->buflen) {
        if (l->len == l->pos) {
            l->buf[l->pos] = c;
            l->pos++;
            l->len++;
	    l->buf[l->len] = '\0';
            if ((!mlmode && l->plen+l->len < l->cols) /* || mlmode */) {
                /* Avoid a full update of the line in the
                 * trivial case. */
		serial_putc(c);
	    } else {
		refreshLine(l);
	    }
    	}
    } else {
            memmove(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);
            l->buf[l->pos] = c;
            l->len++;
            l->pos++;
            l->buf[l->len] = '\0';
	    refreshLine(l);
    }
    return 0;
}

void linenoiseEditMoveLeft(struct linenoiseState *l) {
    if (l->pos > 0) {
        l->pos--;
        refreshLine(l);
    }
}

void linenoiseEditMoveRight(struct linenoiseState *l) {
    if (l->pos != l->len) {
        l->pos++;
        refreshLine(l);
    }
}

void linenoiseEditDelete(struct linenoiseState *l) {
    if (l->len > 0 && l->pos < l->len) {
        memmove(l->buf+l->pos,l->buf+l->pos+1,l->len-l->pos-1);
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

void linenoiseEditBackspace(struct linenoiseState *l) {
    if (l->pos > 0 && l->len > 0) {
        memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
        l->pos--;
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

void linenoiseEditDeletePrevWord(struct linenoiseState *l) {
    size_t old_pos = l->pos;
    size_t diff;

    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos--;
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos--;
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
}

static int linenoiseEdit(char *buf, size_t buflen, const char *prompt)
{
    struct linenoiseState l = {
        .buf = buf, .buflen = buflen,
        .prompt = prompt, .plen = strlen(prompt),
        .oldpos = 0, .pos = 0,
        .len = 0,
        .cols = 80, .maxrows = 0,
        .history_index = 0,
    };

    /* Buffer starts empty. */
    buf[0] = '\0';
    buflen--; /* Make sure there is always space for the nulterm */

    serial_puts(prompt);
    while(1) {
	char c;
	char seq[2] = {0};	
	c = serial_getc();	

	//? -> completionCallback, maybe need to check
	switch(c) {
	case 13:    /* enter */
	    //Handle history
	    return (int)l.len;	    
	case 127:   /* backspace */	
	case 8:     /* ctrl-h */
	    linenoiseEditBackspace(&l);
	    break;
        case 4:     /* ctrl-d, remove char at right of cursor, or of the
                       line is empty, act as end-of-file. */
            if (l.len > 0) {
                linenoiseEditDelete(&l);
            } else {
                //history_len--;
                //free(history[history_len]);
                return -1;
            }
            break;
        case 20:    /* ctrl-t, swaps current character with previous. */
	    //...
	    break;
        case 2:     /* ctrl-b */
            linenoiseEditMoveLeft(&l);
            break;
        case 6:     /* ctrl-f */
            linenoiseEditMoveRight(&l);
            break;
        case 16:    /* ctrl-p */
            //...
            break;
        case 14:    /* ctrl-n */
            //...
            break;
        case 27:    /* escape sequence */
	    //seq[0] = serial_getc(); //Wrong!
	    //seq[1] = serial_getc(); //Wrong!
	    /* Need to implement reading 2 bytes from USART at here */
            if (seq[0] == 91 && seq[1] == 68) {
                /* Left arrow */
                linenoiseEditMoveLeft(&l);
            } else if (seq[0] == 91 && seq[1] == 67) {
                /* Right arrow */
                linenoiseEditMoveRight(&l);
	    }
	default:
	    if(!c) //avoid the NULL byte which received from the USART
		break;
            if (linenoiseEditInsert(&l,c)) return -1;
	    break;
        case 21: /* Ctrl+u, delete the whole line. */
            buf[0] = '\0';
            l.pos = l.len = 0;
            refreshLine(&l);
	    break;
        case 11: /* Ctrl+k, delete from current to end of line. */
            buf[l.pos] = '\0';
            l.len = l.pos;
            refreshLine(&l);
	    break;
        case 1: /* Ctrl+a, go to the start of the line */
            l.pos = 0;
            refreshLine(&l);
	    break;
        case 5: /* ctrl+e, go to the end of the line */
            l.pos = l.len;
            refreshLine(&l);
	    break;
	case 12: /* ctrl+l, clear screen */	
            linenoiseClearScreen();        
            refreshLine(&l);
            break;
	case 23: /* ctrl+w, delete previous word */
	    linenoiseEditDeletePrevWord(&l);
	    break;
	}
    }
    return l.len;
}

static int linenoiseRaw(char *buf, size_t buflen, const char *prompt) {
    int count;

    count = linenoiseEdit(buf, buflen, prompt);
    serial_puts("\n\r");
    
    return count;
}

char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];
    int count;

    linenoiseCallbackFuncInit();

    count = linenoiseRaw(buf,LINENOISE_MAX_LINE,prompt);
    if (count == -1) return NULL;
    return /*strdup(buf);*/ 0; //Need to implement strdup or allocate enough chars
}
