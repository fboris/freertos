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

static void putstr(const char *str) {
    int msg_len = strlen(str);

    int cur;
    for(cur = 0; cur < msg_len; cur++) {
	send_byte(str[cur]);
    }
}

static void read_data(char *buf, size_t count)
{
    int i;
    for(i = 0; i < count; i++) {
	buf[i] = receive_byte();
    }	
}

static int mlmode = 0;  /* Multi line mode. Default is single line. */
static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len = 0;
char **history = NULL;

/* Serial read/write callback functions */
serial_ops serial = {
    .getch = receive_byte,
    .putch = send_byte
};


void linenoiseClearScreen(void) {
    putstr("\x1b[H\x1b[2J");
}

static void freeCompletions(linenoiseCompletions *lc) {
    size_t i;
    for (i = 0; i < lc->len; i++)  
        vPortFree(lc->cvec[i]);    
    if (lc->cvec != NULL)
       vPortFree(lc->cvec);
}


static void linenoiseBeep(void) {
    putstr("\x7");
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
                    stop = 1;
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
    putstr("\x1b[0G");
    /* Write the prompt and the current buffer content */
    putstr(l->prompt);
    putstr(buf);
    /* Erase to right */
    putstr("\x1b[0K");
    /* Move cursor to original position. */
    char sq[] = "\x1b[0G\x1b[00C"; //the max columes of Terminal environment is 80
    /* Set the count of moving cursor */
    sq[6] = (pos+plen) / 10 + 0x30;  
    sq[7] = (pos+plen) % 10 + 0x30;
    putstr(sq);
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

#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1
void linenoiseEditHistoryNext(struct linenoiseState *l, int dir) {
    if (history_len > 1) {
        /* Update the current history entry before to
         * overwrite it with the next one. */
        vPortFree(history[history_len - 1 - l->history_index]);
        history[history_len - 1 - l->history_index] = strdup(l->buf);
        /* Show the new entry */       
        l->history_index += (dir == LINENOISE_HISTORY_PREV) ? 1 : -1;
        if (l->history_index < 0) {    
            l->history_index = 0;          
            return;
        } else if (l->history_index >= history_len) {
            l->history_index = history_len-1;
            return;
        }
        strncpy(l->buf,history[history_len - 1 - l->history_index],l->buflen);
        l->buf[l->buflen-1] = '\0';    
        l->len = l->pos = strlen(l->buf);
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

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    linenoiseHistoryAdd("");
    history_len++;

    putstr(prompt);
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
	    history_len--;
	    vPortFree(history[history_len]);
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
                history_len--;
                vPortFree(history[history_len]);
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
	    linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
            break;
        case 14:    /* ctrl-n */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
            break;
	   /* escape sequence */
	case 27:
	    //Need to merge from "fboris"
	    break;
	default:
	    if(c == '\0') //avoid the NULL byte which received from the USART
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
    putstr("\n\r");
    
    return count;
}

char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];
    int count;

    count = linenoiseRaw(buf,LINENOISE_MAX_LINE,prompt);
    if (count == -1) return NULL;
    return strdup(buf);
}

static void freeHistory(void) {
    if (history) {
        int j;

        for (j = 0; j < history_len; j++)
	    vPortFree(history[j]); 
        vPortFree(history);
    }  
}

int linenoiseHistoryAdd(const char *line) {
    char *linecopy;

    if (history_max_len == 0) return 0;
    if (history == NULL) {
        history = (char **)pvPortMalloc(sizeof(char*)*history_max_len);
        if (history == NULL) return 0; 
        memset(history,0,(sizeof(char*)*history_max_len));
    }
    linecopy = strdup(line);
    if (!linecopy) return 0;
    if (history_len == history_max_len) {
        vPortFree(history[0]);
        memmove(history,history+1,sizeof(char*)*(history_max_len-1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

