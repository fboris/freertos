#include "fio.h"
#include "string.h"
/* Linenoise and shell includes. */
#include "linenoise.h"

extern serial_ops serial;
typedef void (*cmd_func_t)(int argc, char *argv);

struct cmd_t
{
    char *name;
    char *description;
    cmd_func_t handler;
};

typedef struct cmd_t cmd_entry;

static void help_menu(int argc, char *argv);
static void echo_cmd(int argc, char *argv);
static void ps_cmd(int argc, char *argv);
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
static void help_menu(int argc, char *argv)
{}
static void echo_cmd(int argc, char *argv)
{}
static void ps_cmd(int argc, char *argv)
{}
void linenoise_completion(const char *buf, linenoiseCompletions *lc) {
	
    if (buf[0] == 'h') {
	linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello world!");
    }
}

static void proc_cmd(char *cmd)
{
    int i = 0;

    /* process command: it will find and execute availabe commands.*/
    for (i = 0; i < sizeof(available_cmds)/sizeof(cmd_entry); i++) {
        if (strncmp(cmd, available_cmds[i].name, strlen(available_cmds[i].name)) == 0) {
            /*prevent the case echo1. check space here*/
            if( cmd[strlen(available_cmds[i].name)] != ' ')
                continue;
            available_cmds[i].handler(0, cmd);
            return;
        }
    }

    serial.puts("Command not found.\n\r");
}

void shell_task(void *pvParameters)
{
	char *shell_str;
	
	linenoiseSetCompletionCallback(linenoise_completion);

	while(1) {
		shell_str = linenoise("MyShell>> ");
		linenoiseHistoryAdd(shell_str);
        proc_cmd(shell_str);

	}
}
