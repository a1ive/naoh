
#include "prec.h"
#include "zenwinx.h"
#include "commands.h"

winx_command_t winx_command_list;

void
winx_command_register(winx_command_t cmd)
{
	cmd->prev = &winx_command_list;
	if (winx_command_list)
		winx_command_list->prev = &cmd->next;
	cmd->next = winx_command_list;
	winx_command_list = cmd;
}

winx_command_t
winx_command_find(const char* name)
{
	winx_command_t cmd;
	for (cmd = winx_command_list; cmd; cmd = cmd->next)
	{
		if (_stricmp(cmd->name, name) == 0)
			return cmd;
	}
	return NULL;
}

int
winx_command_execute(const char* name, int argc, char** argv)
{
	winx_command_t cmd;
	cmd = winx_command_find(name);
	return (cmd) ? cmd->func(argc, argv) : (-1);
}

static char* expand_environment_variables(char* cmd)
{
	winx_time t;
	wchar_t buffer[16];
	UNICODE_STRING in, out;
	wchar_t* expanded_string;
	char* ret = NULL;
	ULONG number_of_bytes;
	NTSTATUS status;
	wchar_t* cmd16;
	size_t length;
	
	if(cmd == NULL)
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

	/* utf-8 -> ucs2 */
	cmd16 = winx_swprintf(L"%S", cmd);
	/* expand environment variables */
	RtlInitUnicodeString(&in,cmd16);
	out.Length = out.MaximumLength = 0;
	out.Buffer = NULL;
	number_of_bytes = 0;
	status = RtlExpandEnvironmentStrings_U(NULL,
		&in,&out,&number_of_bytes);
	expanded_string = winx_tmalloc(number_of_bytes + sizeof(wchar_t));
	length = (number_of_bytes + sizeof(wchar_t)) / sizeof(wchar_t);
	if(expanded_string)
	{
		RtlInitUnicodeString(&in,cmd16);
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

	/* ucs2 -> utf-8*/
	if (expanded_string)
	{
		ret = winx_sprintf("%S", expanded_string);
		winx_free(expanded_string);
	}
	winx_free(cmd16);
	return ret;
}

int winx_command_parse(char* cmdline)
{
	int i, j, n, argc;
	int arg_detected;
	char *cmdline_copy;
	char **argv = NULL;
	int result;
	winx_command_t p;

	/*
	* Cleanup the command line by removing
	* spaces and newlines from the beginning
	* and the end of the string.
	*/
	while(*cmdline == 0x20 || *cmdline == '\t')
		cmdline ++; /* skip leading spaces */
	n = (int)strlen(cmdline);
	for(i = n - 1; i >= 0; i--)
	{
		if(cmdline[i] != 0x20 && cmdline[i] != '\t' && \
			cmdline[i] != '\n' && cmdline[i] != '\r') break;
		cmdline[i] = 0; /* remove trailing spaces and newlines */
	}

	/*
	* Handle empty lines and comments.
	*/
	if(cmdline[0] == 0 || cmdline[0] == '#')
		return 0;
	
	/*
	* Prepare argc, argv, envp variables.
	* Return immediately if argc == 0.
	*/
	/* a. expand environment variables */
	cmdline_copy = expand_environment_variables(cmdline);
	if(cmdline_copy == NULL)
		return (-1);
	/* b. replace all spaces by zeros */
	n = (int)strlen(cmdline_copy);
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
	argv = winx_malloc(sizeof(char *) * argc);
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
		
	/*
	* Check whether the command 
	* is supported or not.
	*/
	p = winx_command_find(argv[0]);
	
	/*
	* Handle unknown commands.
	*/
	if(p == NULL)
	{
		winx_printf("\nUnknown command: %s!\n\n", cmdline);
		etrace("Unknown command: %s!", cmdline);
		winx_free(argv);
		winx_free(cmdline_copy);
		return 0;
	}
	
	/*
	* Handle the command.
	*/
	result = p->func(argc, argv);
	winx_free(argv);
	winx_free(cmdline_copy);
	return result;
}
