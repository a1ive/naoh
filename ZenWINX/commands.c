/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
* UltraDefrag boot time (native) interface - native commands implementation.
*/

#include "prec.h"
#include "zenwinx.h"
#include "commands.h"

winx_history *p_history = NULL;
PEB* p_peb = NULL;

/**
 * @brief Defines whether @echo is on or not.
 */
int echo_flag = DEFAULT_ECHO_FLAG;

/**
 * @brief Defines whether a command is executed
 * in scripting mode or in interactive mode.
 */
int scripting_mode = 1;

/* help screen */
char *help_message[] =
{
	"Interactive mode commands:",
	"",
	"  echo          - enable/disable command line display or",
	"                  show its status; display a message or",
	"                  an empty line",
	"  call          - execute a boot time script",
	"  exit          - continue Windows boot",
	"  help          - display this help screen",
	"  hexview       - display a file in a HEX viewer layout",
	"  history       - display the list of manually entered commands",
	"  pause         - halt execution for the specified timeout",
	"                  or till a key is pressed",
	"  reboot        - reboot the computer",
	"  set           - list, set or clear environment variables",
	"  shutdown      - shut the computer down",
	"  type          - display a file",
	"",
	NULL
};

typedef int (*cmd_handler_proc)(int argc,wchar_t **argv,wchar_t **envp);
typedef struct
{
	wchar_t *cmd_name;
	cmd_handler_proc cmd_handler;
} cmd_table_entry;

/* forward declarations */
extern cmd_table_entry cmd_table[];
static int type_handler(int argc,wchar_t **argv,wchar_t **envp);

/**
 * @brief help command handler.
 */
static int help_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	if(argc < 1)
		return (-1);
	
	return winx_print_strings(help_message,
		MAX_LINE_WIDTH,MAX_DISPLAY_ROWS,
		DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
		scripting_mode ? 0 : 1);
}

/**
 * @brief history command handler.
 */
static int history_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	winx_history_entry *entry;
	char **strings;
	int i, result;

	if(argc < 1)
		return (-1);

	if(scripting_mode || p_history == NULL || p_history->head == NULL)
		return 0;
	
	/* convert the list of strings to an array */
	strings = winx_malloc((3ULL + p_history->n_entries) * sizeof(char *));
	strings[0] = "Typed commands history:"; strings[1] = ""; i = 2;
	for(entry = p_history->head; i < p_history->n_entries; entry = entry->next)
	{
		if(entry->string)
		{
			strings[i] = entry->string;
			i++;
		}
		if(entry->next == p_history->head) break;
	}
	strings[i] = NULL;
	
	winx_printf("\n");

	result = winx_print_strings(strings,
		MAX_LINE_WIDTH,MAX_DISPLAY_ROWS,
		DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,1);

	winx_free(strings);
	return result;
}

/**
 * @brief echo command handler.
 */
static int echo_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	int i;
	
	if(argc < 1)
		return (-1);

	/* check whether an empty line is requested */
	if(!wcscmp(argv[0],L"echo.")){
		winx_printf("\n");
		return 0;
	}
	
	/* check whether the echo status is requested */
	if(argc < 2){
		if(echo_flag)
			winx_printf("echo is on\n");
		else
			winx_printf("echo is off\n");
		return 0;
	}
	
	/* handle on and off keys */
	if(argc == 2)
	{
		if(!wcscmp(argv[1],L"on"))
		{
			echo_flag = 1;
			return 0;
		}
		else if(!wcscmp(argv[1],L"off"))
		{
			echo_flag = 0;
			return 0;
		}
	}
	
	/* handle the echo command */
	for(i = 1; i < argc; i++)
	{
		winx_printf("%ws",argv[i]);
		if(i != argc - 1)
			winx_printf(" ");
	}
	winx_printf("\n");
	return 0;
}

/**
 * @brief type command handler.
 */
static int type_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	wchar_t path[MAX_PATH + 1];
	wchar_t *filename;
	int i, length;
	size_t filesize;
	char *buffer, *second_buffer;
	int unicode_detected;
	char *strings[] = { NULL, NULL };
	int result;
	
	if(argc < 1)
		return (-1);

	if (argc < 2)
	{
		winx_printf("\n%ws: file name must be specified\n\n", argv[0]);
		return (-1);
	}

	length = 0;
	for(i = 1; i < argc; i++)
		length += (int)wcslen(argv[i]) + 1;
	filename = winx_malloc(length * sizeof(wchar_t));
	filename[0] = 0;
	for(i = 1; i < argc; i++)
	{
		wcscat(filename,argv[i]);
		if(i != argc - 1)
			wcscat(filename,L" ");
	}
	(void)_snwprintf(path,MAX_PATH,L"\\??\\%ws",filename);
	path[MAX_PATH] = 0;
	winx_free(filename);

	(void)filename;

	/* read file contents entirely */
	buffer = winx_get_file_contents(path,&filesize);
	if(buffer == NULL)
		return 0; /* the file is empty or some error */
	
	/* terminate the buffer by two zeros */
	buffer[filesize] = buffer[filesize + 1] = 0;

	/* check for UTF-16 signature which exists in files edited in Notepad */
	unicode_detected = 0;
	if(filesize >= sizeof(wchar_t))
	{
		if((unsigned char)buffer[0] == 0xFF && (unsigned char)buffer[1] == 0xFE)
			unicode_detected = 1;
	}

	/* print file contents */
	if(unicode_detected)
	{
		second_buffer = winx_tmalloc(filesize + 1);
		if(second_buffer == NULL)
		{
			winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
				argv[0],filesize + 1);
			winx_release_file_contents(buffer);
			return (-1);
		}
		(void)_snprintf(second_buffer,filesize + 1,"%ws",(wchar_t *)(buffer + 2));
		second_buffer[filesize] = 0;
		strings[0] = second_buffer;
		result = winx_print_strings(strings,MAX_LINE_WIDTH,
			MAX_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
			scripting_mode ? 0 : 1);
		winx_free(second_buffer);
	}
	else
	{
		strings[0] = buffer;
		result = winx_print_strings(strings,MAX_LINE_WIDTH,
			MAX_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
			scripting_mode ? 0 : 1);
	}
	
	/* cleanup */
	winx_release_file_contents(buffer);
	return result;
}

/**
 * @brief hexview command handler.
 */
static int hexview_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	wchar_t path[MAX_PATH + 1];
	wchar_t *filename;
	int i, length;
	WINX_FILE *f;
	ULONGLONG size;
	size_t filesize;
	size_t bytes_to_read, bytes_to_print, n, j, k, m;
	#define SCREEN_BUFFER_SIZE (8 * (MAX_DISPLAY_ROWS - 5))
	unsigned char buffer[SCREEN_BUFFER_SIZE];
	int result;
	char *offset;
	KBD_RECORD kbd_rec;
	int escape_detected = 0;
	int break_detected = 0;
	char esq[] = {'\a', '\b', '\f', '\n', '\r', '\t', '\v', 0};
	int esq_found = 0;
	
	if(argc < 1)
		return (-1);

	if(argc < 2)
	{
		winx_printf("\n%ws: file name must be specified\n\n",argv[0]);
		return (-1);
	}

	/* build the native path to the file */
	length = 0;
	for(i = 1; i < argc; i++)
		length += (int)wcslen(argv[i]) + 1;
	filename = winx_malloc(length * sizeof(wchar_t));
	filename[0] = 0;
	for(i = 1; i < argc; i++)
	{
		wcscat(filename,argv[i]);
		if(i != argc - 1)
			wcscat(filename,L" ");
	}
	(void)_snwprintf(path,MAX_PATH,L"\\??\\%ws",filename);
	path[MAX_PATH] = 0;
	winx_free(filename);
	
	/* open the file */
	f = winx_fopen(path,"r");
	if(f == NULL)
	{
		winx_printf("\n%ws: cannot open %ws\n\n",argv[0],path);
		return (-1);
	}
	size = winx_fsize(f);
	if(size == 0)
	{
		winx_fclose(f);
		return 0; /* nothing to display */
	}
#ifndef _WIN64
	if(size > 0xFFFFFFFF)
	{
		winx_printf("\n%ws: files larger than ~4GB aren\'t supported\n\n",argv[0]);
		winx_fclose(f);
		return (-1);
	}
#endif
	filesize = (size_t)size;
	
	/* read the file by portions needed to fill a single screen */
	offset = 0x0;
	while(filesize)
	{
		bytes_to_read = min(SCREEN_BUFFER_SIZE,filesize);
		result = (int)winx_fread(buffer,sizeof(char),bytes_to_read,f);
		if(result != bytes_to_read && winx_fsize(f) == size)
		{
			winx_printf("\n%ws: cannot read %ws\n\n",argv[0],path);
			winx_fclose(f);
			return (-1);
		}
		/* fill the screen */
		bytes_to_print = bytes_to_read;
		j = 0;
		while(bytes_to_print)
		{
			n = min(8, bytes_to_print);
			winx_printf("%p: ",offset);
			for(k = 0; k < n; k++)
				winx_printf("%02x ",(UINT)buffer[j+k]);
			for(; k < 8; k++)
				winx_printf("   ");
			winx_printf("| ");
			for(k = 0; k < n; k++)
			{
				/* replace escape sequences and 0x0 codes by spaces */
				esq_found = 0;
				for(m = 0; esq[m]; m++)
				{
					if(esq[m] == buffer[j+k])
					{
						esq_found = 1;
						break;
					}
				}
				if(esq_found || buffer[j+k] == 0x0)
					winx_printf(" ");
				else
					winx_printf("%c",buffer[j+k]);
			}
			for(; k < 8; k++)
				winx_printf(" ");
			winx_printf("\n");
			offset += n;
			j += n;
			bytes_to_print -= n;
		}
		/* go to the next portion of data */
		filesize -= bytes_to_read;
		if(filesize && !scripting_mode)
		{
			/* display prompt to hit any key in the interactive mode */
			winx_printf("\n%s\n\n",DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY);
			/* wait for any key */
			if(winx_kb_read(&kbd_rec,INFINITE) < 0)
			{
				break; /* break in case of errors */
			}
			/* check for escape */
			if(kbd_rec.wVirtualScanCode == 0x1)
			{
				escape_detected = 1;
			}
			else if(kbd_rec.wVirtualScanCode == 0x1d)
			{
				/* distinguish between control keys and the break key */
				if(!(kbd_rec.dwControlKeyState & LEFT_CTRL_PRESSED) && \
				  !(kbd_rec.dwControlKeyState & RIGHT_CTRL_PRESSED))
				{
					break_detected = 1;
				}
			}
			if(escape_detected || break_detected)
				break;
		}
	}

	/* cleanup */
	winx_fclose(f);
	return 0;
}

/**
 * @brief Enumerates all environment variables.
 */
static int list_environment_variables(int argc,wchar_t **argv,wchar_t **envp)
{
	char **strings;
	int i, j, n, length;
	int result;
	int filter_strings = 0;
	
	if(argc < 1 || envp == NULL)
		return (-1);
	
	if(envp[0] == NULL)
		return 0; /* nothing to print */
	
	if(argc > 1)
		filter_strings = 1;
	
	/* convert envp to an array of ANSI strings */
	for(n = 0; envp[n] != NULL; n++) {}
	strings = winx_malloc((1ULL + n) * sizeof(char *));
	RtlZeroMemory((void *)strings,(1ULL + n) * sizeof(char *));
	for(i = 0, j = 0; i < n; i++)
	{
		if(filter_strings && winx_wcsistr(envp[i],argv[1]) != (wchar_t *)envp[i])
			continue;
		length = (int)wcslen(envp[i]);
		strings[j] = winx_malloc((1ULL + length) * sizeof(char));
		(void)_snprintf(strings[j],1ULL + length,"%ws",envp[i]);
		strings[j][length] = 0;
		j++;
	}
	/* print strings */
	result = winx_print_strings(strings,MAX_LINE_WIDTH,
		MAX_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
		scripting_mode ? 0 : 1);
	/* cleanup */
	for(i = 0; i < n; i++)
		winx_free(strings[i]);
	winx_free(strings);
	return result;

	for(i = 0; i < n; i++)
		winx_free(strings[i]);
	winx_free(strings);
	return (-1);
}

/**
 * @brief set command handler.
 */
static int set_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	int name_length = 0, value_length = 0;
	wchar_t *name = NULL, *value = NULL;
	int i, j, n, result;
	
	if(argc < 1)
		return (-1);

	/*
	* Check whether the environment variables
	* listing is requested or not.
	*/
	if(argc < 2)
	{
		/* list all environment variables */
		return list_environment_variables(argc,argv,envp);
	}
	else
	{
		/* check whether the first parameter contains the '=' character */
		if(!wcschr(argv[1],'='))
		{
			/*
			* List variables containing argv[1] string
			* in the beginning of their name.
			*/
			return list_environment_variables(argc,argv,envp);
		}
		/* calculate name and value lengths */
		n = (int)wcslen(argv[1]);
		for(i = 0; i < n; i++)
		{
			if(argv[1][i] == '=')
			{
				name_length = i;
				value_length = n - i - 1;
				break;
			}
		}
		/* validate the '=' character position */
		if(name_length == 0 || (value_length == 0 && argc >= 3))
		{
			winx_printf("\n%ws: invalid syntax\n\n",argv[0]);
			return (-1);
		}
		/* append all the remaining parts of the value string */
		for(i = 2; i < argc; i++)
			value_length += 1 + (int)wcslen(argv[i]);
		/* allocate memory */
		name = winx_malloc((1ULL + name_length) * sizeof(wchar_t));
		if(value_length)
			value = winx_malloc((1ULL + value_length) * sizeof(wchar_t));
		/* extract name and value */
		n = (int)wcslen(argv[1]);
		for(i = 0; i < n; i++)
		{
			if(argv[1][i] == '=')
				break;
			name[i] = argv[1][i];
		}
		name[i] = 0;
		if(value_length)
		{
			for(i++, j = 0; i < n; i++)
			{
				value[j] = argv[1][i];
				j++;
			}
			value[j] = 0;
			for(i = 2; i < argc; i++)
			{
				wcscat(value,L" ");
				wcscat(value,argv[i]);
			}
		}
		if(value_length)
		{
			/* set the environment variable */
			result = winx_setenv(name,value);
			winx_free(value);
		}
		else
		{
			/* clear the environment variable */
			result = winx_setenv(name,NULL);
		}
		winx_free(name);
		return result;
	}
	
	return 0; /* this point will never be reached */
}

/**
 * @brief pause command handler.
 */
static int pause_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	int msec;

	if(argc < 1)
		return (-1);

	/* check whether "Hit any key to continue..." prompt is requested */
	if(argc < 2)
	{
		winx_printf("%s",PAUSE_MESSAGE);
		(void)winx_kbhit(INFINITE);
		winx_printf("\n\n");
		return 0;
	}
	
	/* pause execution for the specified time interval */
	msec = (int)_wtol(argv[1]);
	winx_sleep(msec);
	return 0;
}

/**
 * @brief shutdown command handler.
 */
static int shutdown_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	winx_printf("Shutdown ...\n");
	winx_shutdown();
	winx_printf("\nShutdown your computer manually.\n");
	return 0;
}

/**
 * @brief reboot command handler.
 */
static int reboot_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	winx_printf("Reboot ...\n");
	winx_reboot();
	winx_printf("\nReboot your computer manually.\n");
	return 0;
}

/**
 * @brief exit command handler.
 */
static int exit_handler(int argc,wchar_t **argv,wchar_t **envp)
{
	int exit_code = 0;

	if(argc > 1)
		exit_code = _wtoi(argv[1]);

	if (p_history)
		winx_destroy_history(p_history);
	winx_exit(exit_code);
	return 0;
}

/**
 * @brief call command handler.
 */
static int call_handler(int argc, wchar_t** argv, wchar_t** envp)
{
	wchar_t* filename;
	int i, length;
	int result;
	int old_scripting_mode;

	if (argc < 1)
		return (-1);

	old_scripting_mode = scripting_mode;

	if (argc < 2)
	{
		result = winx_process_script(NULL, NULL, p_peb);
	}
	else
	{
		length = 0;
		for (i = 1; i < argc; i++)
			length += (int)wcslen(argv[i]) + 1;
		filename = winx_malloc(length * sizeof(wchar_t));
		filename[0] = 0;
		for (i = 1; i < argc; i++)
		{
			wcscat(filename, argv[i]);
			if (i != argc - 1)
				wcscat(filename, L" ");
		}
		result = winx_process_script(p_history, filename, p_peb);
		winx_free(filename);
	}

	scripting_mode = old_scripting_mode;
	return result;
}

/**
 * @brief Expands environment variables.
 * @details %DATE% will be replaced by 
 * Year-Month-Day, 2011-08-01 for example.
 * %TIME% will be replaced by Hour-Minute,
 * 12-57 for example.
 */
static wchar_t *expand_environment_variables(wchar_t *command)
{
	winx_time t;
	wchar_t buffer[16];
	UNICODE_STRING in, out;
	wchar_t *expanded_string;
	ULONG number_of_bytes;
	NTSTATUS status;
	int length;
	
	if(command == NULL)
		return NULL;
	
	/* set %DATE% and %TIME% */
	memset(&t,0,sizeof(winx_time));
	(void)winx_get_local_time(&t);
	_snwprintf(buffer,sizeof(buffer)/sizeof(wchar_t),
		L"%04i-%02i-%02i",(int)t.year,(int)t.month,(int)t.day);
	buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
	if(winx_setenv(L"DATE",buffer) < 0)
	{
		etrace("cannot set %%DATE%% environment variable");
		winx_printf("\ncannot set %%DATE%% environment variable\n\n");
	}
	_snwprintf(buffer,sizeof(buffer)/sizeof(wchar_t),
		L"%02i-%02i",(int)t.hour,(int)t.minute);
	buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
	if(winx_setenv(L"TIME",buffer) < 0)
	{
		etrace("cannot set %%TIME%% environment variable");
		winx_printf("\ncannot set %%TIME%% environment variable\n\n");
	}

	/* expand environment variables */
	RtlInitUnicodeString(&in,command);
	out.Length = out.MaximumLength = 0;
	out.Buffer = NULL;
	number_of_bytes = 0;
	status = RtlExpandEnvironmentStrings_U(NULL,
		&in,&out,&number_of_bytes);
	expanded_string = winx_tmalloc(number_of_bytes + sizeof(wchar_t));
	length = (number_of_bytes + sizeof(wchar_t)) / sizeof(wchar_t);
	if(expanded_string)
	{
		RtlInitUnicodeString(&in,command);
		out.Length = out.MaximumLength = (USHORT)number_of_bytes;
		out.Buffer = expanded_string;
		status = RtlExpandEnvironmentStrings_U(NULL,
			&in,&out,&number_of_bytes);
		if(NT_SUCCESS(status))
		{
			expanded_string[length - 1] = 0;
		}
		else
		{
			strace(status,"cannot expand environment variables");
			winx_printf("\ncannot expand environment variables\n\n");
			winx_free(expanded_string);
			expanded_string = NULL;
		}
	}
	else
	{
		etrace("cannot allocate %u bytes of memory",
			number_of_bytes + sizeof(wchar_t));
		winx_printf("\ncannot allocate %u bytes of memory\n\n",
			number_of_bytes + sizeof(wchar_t));
	}
	
	/* clear %DATE% and %TIME% */
	(void)winx_setenv(L"DATE",NULL);
	(void)winx_setenv(L"TIME",NULL);
	return expanded_string;
}

/**
 * @brief The list of supported commands.
 */
cmd_table_entry cmd_table[] =
{
	{ L"echo",      echo_handler },
	{ L"call",      call_handler },
	{ L"exit",      exit_handler },
	{ L"help",      help_handler },
	{ L"hexview",   hexview_handler },
	{ L"history",   history_handler },
	{ L"pause",     pause_handler },
	{ L"reboot",    reboot_handler },
	{ L"set",       set_handler },
	{ L"shutdown",  shutdown_handler },
	{ L"type",      type_handler },
	{ L"",          NULL }
};

/**
 * @brief Executes a command.
 * @param[in] cmdline the command line.
 * @return Zero for success, negative 
 * value otherwise.
 */
int winx_parse_command(winx_history *history, wchar_t *cmdline, PEB* peb)
{
	int i, j, n, argc;
	int at_detected = 0;
	int arg_detected;
	wchar_t *full_cmdline;
	wchar_t *cmdline_copy;
	wchar_t **argv = NULL;
	wchar_t **envp = NULL;
	wchar_t *string;
	int length;
	int result;

	p_history = history;
	p_peb = peb;
	/*
	* Cleanup the command line by removing
	* spaces and newlines from the beginning
	* and the end of the string.
	*/
	while(*cmdline == 0x20 || *cmdline == '\t')
		cmdline ++; /* skip leading spaces */
	n = (int)wcslen(cmdline);
	for(i = n - 1; i >= 0; i--)
	{
		if(cmdline[i] != 0x20 && cmdline[i] != '\t' && \
			cmdline[i] != '\n' && cmdline[i] != '\r') break;
		cmdline[i] = 0; /* remove trailing spaces and newlines */
	}
	full_cmdline = cmdline;
	
	/*
	* Skip @ in the beginning of the line.
	*/
	if(cmdline[0] == '@')
	{
		at_detected = 1;
		cmdline++;
	}
	
	/*
	* Handle empty lines and comments.
	*/
	if(cmdline[0] == 0 || cmdline[0] == ';' || cmdline[0] == '#')
	{
		if(echo_flag && !at_detected)
			winx_printf("%ws\n",cmdline);
		return 0;
	}
	
	/*
	* Prepare argc, argv, envp variables.
	* Return immediately if argc == 0.
	*/
	/* a. expand environment variables */
	cmdline_copy = expand_environment_variables(cmdline);
	if(cmdline_copy == NULL)
		return (-1);
	/* b. replace all spaces by zeros */
	n = (int)wcslen(cmdline_copy);
	argc = 1;
	for(i = 0; i < n; i++)
	{
		if(cmdline_copy[i] == 0x20 || cmdline_copy[i] == '\t')
		{
			cmdline_copy[i] = 0;
			if(cmdline_copy[i+1] != 0x20 && cmdline_copy[i+1] != '\t')
				argc ++;
		}
	}
	/* c. allocate memory for the argv array */
	argv = winx_malloc(sizeof(wchar_t *) * argc);
	/* d. fill the argv array */
	j = 0; arg_detected = 0;
	for(i = 0; i < n; i++)
	{
		if(cmdline_copy[i])
		{
			if(!arg_detected)
			{
				argv[j] = cmdline_copy + i;
				j ++;
				arg_detected = 1;
			}
		} else {
			arg_detected = 0;
		}
	}
	/* e. build environment */
	envp = NULL;
	if(peb)
	{
		if(peb->ProcessParameters)
		{
			if(peb->ProcessParameters->Environment)
			{
				/* build an array of unicode strings */
				string = peb->ProcessParameters->Environment;
				for(n = 0; ; n++)
				{
					/* empty line indicates the end of environment */
					if(string[0] == 0) break;
					length = (int)wcslen(string);
					string += 1ULL + length;
				}
				if(n > 0)
				{
					envp = winx_malloc((1ULL + n) * sizeof(wchar_t *));
					RtlZeroMemory((void *)envp,(1ULL + n) * sizeof(wchar_t *));
					string = peb->ProcessParameters->Environment;
					for(i = 0; i < n; i++)
					{
						/* empty line indicates the end of environment */
						if(string[0] == 0) break;
						envp[i] = string;
						length = (int)wcslen(string);
						string += 1ULL + length;
					}
				}
			}
		}
	}
	
	/*
	* Print command line on the screen whenever
	* it has no @ sign in the beginning and echo
	* is on. Besides, never print echo commands
	* themselves.
	*/
	if(echo_flag && !at_detected && wcscmp(argv[0],L"echo") && wcscmp(argv[0],L"echo."))
		winx_printf("%ws\n",cmdline);
	
	/*
	* Check whether the command 
	* is supported or not.
	*/
	for(i = 0; cmd_table[i].cmd_handler != NULL; i++)
		if(!wcscmp(argv[0],cmd_table[i].cmd_name))
			break;
	
	/*
	* Handle unknown commands.
	*/
	if(cmd_table[i].cmd_handler == NULL)
	{
		winx_printf("\nUnknown command: %ws!\n\n",full_cmdline);
		etrace("Unknown command: %ws!",full_cmdline);
		winx_free(argv);
		winx_free(envp);
		winx_free(cmdline_copy);
		return 0;
	}
	
	/*
	* Handle the command.
	*/
	result = cmd_table[i].cmd_handler(argc,argv,envp);
	winx_free(argv);
	winx_free(envp);
	winx_free(cmdline_copy);
	return result;
}
