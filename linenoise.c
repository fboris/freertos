#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "linenoise.h"  

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 4096

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

static int linenoiseEdit(char *buf, size_t buflen, const char *prompt)
{
    struct linenoiseState l;

    l.buf = buf;
    l.buflen = buflen;
    l.prompt = prompt;
    l.plen = strlen(prompt);
    l.oldpos = l.pos = 0;
    l.len = 0;
    l.cols = getColumns();
    l.maxrows = 0;
    l.history_index = 0;

    while(1) {
	char c;

	//c = receive_data();

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
            //...
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

    if (buflen == 0) {
        errno = EINVAL;
        return -1;
    }

    //Read from USART

    count = linenoiseEdit(buf, buflen, prompt);
    //print new line
}

char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];
    int count;

    count = linenoiseRaw(buf,LINENOISE_MAX_LINE,prompt);
    if (count == -1) return NULL;
    return strdup(buf);
}
