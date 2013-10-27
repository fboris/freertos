#ifndef __SHELL_H__
#define __SHELL_H__

/* Enumeration for command types. */
enum CMD_TYPE{
	CMD_HELP = 0,
	CMD_ECHO,
	CMD_PS,
	CMD_TEST,
	CMD_CALLHOST
};

void shell_task(void *pvParameters);

#endif
