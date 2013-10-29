#ifndef __SHELL_H__
#define __SHELL_H__

/* Enumeration for command types. */
enum CMD_TYPE{
	CMD_HELP = 0,
	CMD_ECHO,
	CMD_PS,
	CMD_TEST,
	CMD_CALLHOST,
	CMD_HISTORY
};
/* Enumeration for command return types. */
typedef enum CMD_RETURN_TYPE{
	CMD_SUCCESS = 0,
	CMD_FAILED = -1
}CMD_RETURN_TYPE;
void shell_task(void *pvParameters);

#endif
