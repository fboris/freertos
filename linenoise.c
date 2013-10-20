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

static linenoiseCompletionCallback *completionCallback = NULL;

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

static void refreshLine(struct linenoiseState *l);

/* USART read/write functions structure */
typedef struct {
    char (*getch)(); //If declare as getc will cause naming conflict
    void (*putch)(char ch); //If declare as putc will cause naming conflict
} serial_ops;




static int mlmode = 0;  /* Multi line mode. Default is single line. */

/* Serial read/write callback functions */
serial_ops serial = {
    .getch = receive_byte,
    .putch = send_byte
};


void linenoiseClearScreen(void) {
    puts("\x1b[H\x1b[2J");
}

static void freeCompletions(linenoiseCompletions *lc) {
    size_t i;
    for (i = 0; i < lc->len; i++)  
        vPortFree(lc->cvec[i]);    
    if (lc->cvec != NULL)
       vPortFree(lc->cvec);
}


static void linenoiseBeep(void) {
    puts("\x7");
}

static int completeLine(struct linenoiseState *ls) {
    linenoiseCompletions lc = { 0, NULL };
    int nwritten = 0;
    char c = 0;

    completionCallback(ls->buf,&lc);
    if (lc.len == 0) {
        linenoiseBeep();
    } else {
	    size_t stop = 0, i = 0;

    	while(!stop) {
    	     /* Show completion or original buffer */
            if (i < lc.len) {
                struct linenoiseState saved = *ls;

                ls->len = ls->pos = strlen(lc.cvec[i]);
                ls->buf = lc.cvec[i];
                refreshLine(ls);
                ls->len = saved.len;
                ls->pos = saved.pos;
                ls->buf = saved.buf;
            } else {
                refreshLine(ls);
            }

	       c = serial.getch();

            switch(c) {
                case 9: /* tab */
                    i = (i+1) % (lc.len+1);
                    if (i == lc.len) linenoiseBeep();
                    break;
                case 27: /* escape */
                    /* Re-show original buffer */
                    if (i < lc.len) refreshLine(ls);
                    //stop = 1;
                    break;
                default:
                    /* Update buffer and return */
                    if (i < lc.len) {
            			for(nwritten = 0; nwritten < ls->buflen; nwritten++) {
            				ls->buf[nwritten] = lc.cvec[i][nwritten];
            				if(lc.cvec[i][nwritten] == '\0')
            					break;	
            			}

                            ls->len = ls->pos = nwritten;
                    }
                    stop = 1;
                    break;
            }
        }
    }

    freeCompletions(&lc);
    return c; /* Return last read character */
}

void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn) {
    completionCallback = fn;
}

void linenoiseAddCompletion(linenoiseCompletions *lc, char *str) {     
    size_t len = strlen(str);
    char *copy = (char *)pvPortMalloc(len+1);
    memcpy(copy,str,len+1);
    lc->cvec = (char**)pvPortRealloc(lc->cvec,sizeof(char*)*(lc->len+1));
    lc->cvec[lc->len++] = copy;                                        
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
    puts("\x1b[0G");
    /* Write the prompt and the current buffer content */
    puts(l->prompt);
    puts(buf);
    /* Erase to right */
    puts("\x1b[0K");
    /* Move cursor to original position. */
    char sq[] = "\x1b[0G\x1b[12C"; //the max columes of Terminal environment is 80
    /* Set the count of moving cursor */
    sq[6] = (pos+plen) / 10 + 0x30;  
    sq[7] = (pos+plen) % 10 + 0x30;
    puts(sq);
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
        		serial.putch(c);

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

    puts(prompt);
    while(1) {
    	char c;
    	char seq[2] = {0};	
    	c = serial.getch();	

    	/* Only autocomplete when the callback is set. */
        if (c == 9 && completionCallback != NULL) {
            c = completeLine(&l);        
            /* Return on errors */
            if (c < 0) return l.len; 
            /* Read next character when 0 */
            if (c == 0) continue;      
        }

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
    puts("\n\r");
    
    return count;
}

char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];
    int count;

    count = linenoiseRaw(buf,LINENOISE_MAX_LINE,prompt);
    if (count == -1) return NULL;
    return /*strdup(buf);*/ 0; //Need to implement strdup or allocate enough chars
}
