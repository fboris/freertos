#include "fio.h"
#include "string.h"
/* Linenoise and shell includes. */
#include "linenoise.h"
#include "shell.h"

typedef int (*cmd_func_t)(int argc, char *argv);

struct cmd_t
{
    char *name;
    char *description;
    cmd_func_t handler;
};

typedef struct cmd_t cmd_entry;

static int help_menu(int argc, char *argv);
static int echo_cmd(int argc, char *argv);
static int ps_cmd(int argc, char *argv);
static cmd_entry available_cmds[] = {
        [CMD_HELP] = {.name = "help",.description  = "Show availabe commands.", .handler = help_menu},
        [CMD_ECHO] =  {.name = "echo",.description  = "Show words you input.",.handler = echo_cmd },
        [CMD_PS] = {.name = "ps",.description  = "List All current process.",.handler = ps_cmd}
  
};
static int help_menu(int argc, char *argv)
{
     /*help only support list all commands now.*/
    if( strlen(argv) != strlen(available_cmds[CMD_HELP].name))
        return -1;
    int i;
    puts("Name\tDescription\r\n");
    for (i = 0;i < sizeof(available_cmds)/sizeof(cmd_entry) ; i++){
        puts(available_cmds[i].name);
        puts("\t");
        puts(available_cmds[i].description);
        puts("\r\n");
    }
    return 0;
}
static int echo_cmd(int argc, char *argv)
{
    /*There must be  a space after "echo" string*/
    if( argv[strlen(available_cmds[CMD_ECHO].name)] != ' ')
        return -1;
    argv += strlen(available_cmds[CMD_ECHO].name) + 1;

    for(; *argv; argv++){
        if ( *argv != ' '){
            
            break;
        }
    }
    puts(argv);
    puts("\r\n");
    return 0; 
}
static int ps_cmd(int argc, char *argv)
{
    char buf[1024]={0};
    vTaskList(buf);
    puts(buf);
    return 0;
}
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
            /*check command*/
            if(available_cmds[i].handler(0, cmd) == -1)
                continue;
            return;
        }
    }

    puts("Command not found.\n\r");
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
