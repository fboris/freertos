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

char (*serial_getc)() = receive_byte;
void (*serial_send_str)(const char *str) = queue_str_task;

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
    serial_send_str("\x1b[0G");
    /* Write the prompt and the current buffer content */
    serial_send_str(l->prompt);
    serial_send_str(buf);
    /* Erase to right */
    serial_send_str("\x1b[0K");
    /* Move cursor to original position. */
    char sq[] = "\x1b[0G\x1b[0C";
    sq[10] = (int)(pos+plen);  //Set the count of moving cursor
    serial_send_str(sq);
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
		char buf_char[2] = {'\0'};
		buf_char[0] = c;
		serial_send_str(buf_char);
	    } else {
		refreshLine(l);
	    }
    	}
    } else {
            //memmove(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);   //Maybe need to implement this function
            l->buf[l->pos] = c;
            l->len++;
            l->pos++;
            l->buf[l->len] = '\0';
	    refreshLine(l);
    }
    return 0;
}

static int linenoiseEdit(char *buf, size_t buflen, const char *prompt)
{
    struct linenoiseState l;

    l.buf = buf;
    l.buflen = buflen;
    l.prompt = prompt;
    l.plen = strlen(prompt);
    l.oldpos = l.pos = 0;
    l.len = 0;
    l.cols = 80;
    l.maxrows = 0;
    l.history_index = 0;

    /* Buffer starts empty. */
    buf[0] = '\0';
    buflen--; /* Make sure there is always space for the nulterm */

    serial_send_str(prompt);
    while(1) {
	char c;
	char buf_char[2] = {'\0'};
	
	c = serial_getc();	
	buf_char[0] = c;

	//? -> completionCallback, maybe need to check
	switch(c) {
	case 13:    /* enter */
	    //Handle history
	    return (int)l.len;	    
	case 127:   /* backspace */	
	case 8:     /* ctrl-h */
	    //linenoiseEditBackspace(&l);
        case 4:     /* ctrl-d, remove char at right of cursor, or of the
                       line is empty, act as end-of-file. */
	    //...
	    break;
        case 20:    /* ctrl-t, swaps current character with previous. */
	    //...
	    break;
        case 2:     /* ctrl-b */
            //...
            break;
        case 6:     /* ctrl-f */
            //...
            break;
        case 16:    /* ctrl-p */
            //...
            break;
        case 14:    /* ctrl-n */
            //...
            break;
        case 27:    /* escape sequence */
	    // ...
	default:
	    if(!c) //avoid the NULL byte received from the USART
		break;
            if (linenoiseEditInsert(&l,c)) return -1;
	    break;
        case 21: /* Ctrl+u, delete the whole line. */
	    //...
	    break;
        case 11: /* Ctrl+k, delete from current to end of line. */
	    //...
	    break;
        case 1: /* Ctrl+a, go to the start of the line */
	    //...
	    break;
        case 5: /* ctrl+e, go to the end of the line */
	    //...
	    break;
	case 12: /* ctrl+l, clear screen */	
	    //...
	    break;
	case 23: /* ctrl+w, delete previous word */
	    //...
	    break;
	}
    }
    return l.len;
}

static int linenoiseRaw(char *buf, size_t buflen, const char *prompt) {
    int count;

    count = linenoiseEdit(buf, buflen, prompt);
    serial_send_str("\n\r");
    
    return count;
}

char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];
    int count;

    count = linenoiseRaw(buf,LINENOISE_MAX_LINE,prompt);
    if (count == -1) return NULL;
    return /*strdup(buf);*/ 0; //Need to implement strdup or allocate enough chars
}
