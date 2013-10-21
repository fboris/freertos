#include "fio.h"
#include "string.h"
/* Linenoise and shell includes. */
#include "linenoise.h"
typedef void (*cmd_func_t)(void);

struct cmd_t
{
    char *name;
    char *description;
    cmd_func_t handler;
};

typedef struct cmd_t cmd_entry;

static void help_menu(void);
static void echo_cmd(void);
static void ps_cmd(void);
static cmd_entry available_cmds[] = {
        {
            .name = "help",
            .description  = "Show availabe commands.",
            .handler = help_menu
        },
        {
            .name = "echo",
            .description  = "Show words you input.",
            .handler = echo_cmd
        },
        {
            .name = "ps",
            .description  = "List All current process.",
            .handler = ps_cmd
        }
  
};
static void help_menu(void)
{}
static void echo_cmd(void)
{}
static void ps_cmd(void)
{}
void linenoise_completion(const char *buf, linenoiseCompletions *lc) {
	
    if (buf[0] == 'h') {
	linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello world!");
    }
}

void shell_task(void *pvParameters)
{
	char *shell_str;
	
	linenoiseSetCompletionCallback(linenoise_completion);

	while(1) {
		shell_str = linenoise("linenoise > ");
		linenoiseHistoryAdd(shell_str);
		
	}
}
