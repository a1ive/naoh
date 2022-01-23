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

typedef struct _object_path
{
	struct _object_path* next;
	struct _object_path* prev;
	wchar_t path[MAX_LONG_PATH + 1];
	int processed;
} object_path;

int winx_parse_command(winx_history* history, wchar_t* cmdline, PEB* peb);

int winx_process_script(winx_history* history, wchar_t* filename, PEB* peb);

#endif