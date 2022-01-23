#ifndef _ZENWINX_COMMANDS_H_
#define _ZENWINX_COMMANDS_H_

/* define how many lines to display for each text page,
   the smallest boot screen height is 24 rows,
   which must be reduced by one row for the prompt */
#define MAX_DISPLAY_ROWS 23

   /* define how many characters may be
	  printed on a line after the prompt */
#define MAX_LINE_WIDTH 70

	  /* message to terminate the volume processing */
#define BREAK_MESSAGE "Use Pause/Break key to abort the process early.\n\n"

/* define whether @echo is on by default or not */
#define DEFAULT_ECHO_FLAG 1

/* message to be shown when the pause command is used without parameters */
#define PAUSE_MESSAGE "Hit any key to continue..."

#define MAX_ENV_VARIABLE_LENGTH 32766
#define MAX_LONG_PATH MAX_ENV_VARIABLE_LENGTH /* must be equal */

typedef int(*winx_command_func_t) (int argc, char** argv);

struct winx_command
{
	struct winx_command* next;
	struct winx_command** prev;
	const char* name;
	winx_command_func_t func;
	const char* help;
};
typedef struct winx_command* winx_command_t;

extern winx_command_t winx_command_list;

void
winx_command_register(winx_command_t cmd);

winx_command_t
winx_command_find(const char* name);

int
winx_command_execute(const char* name, int argc, char** argv);

int winx_command_parse(char* cmdline);

#endif