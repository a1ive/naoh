
#include "naoh.h"

/* help */
static int cmd_help_func(int argc, char** argv)
{
	int i;
	winx_command_t p;
	if (argc < 2)
	{
		winx_printf("Available commands:");
		for (p = winx_command_list; p; p = p->next)
			winx_printf(" %s", p->name);
		winx_printf("\n");
		return 0;
	}
	for (i = 1; i < argc; i++)
	{
		p = winx_command_find(argv[i]);
		if (p)
			winx_printf("%s\n\n", p->help);
		else
			winx_printf("Unknown command %s\n\n", argv[i]);
	}
	return 0;
}

static struct winx_command cmd_help =
{
	.next = 0,
	.name = "help",
	.func = cmd_help_func,
	.help = "help [COMMAND] ...\nPrint help text.",
};

/* echo */
static int cmd_echo_func(int argc, char** argv)
{
	int i;
	for (i = 1; i < argc; i++)
		winx_printf("%s%s", i == 1 ? "" : " ", argv[i]);
	winx_printf("\n");
	return 0;
}

static struct winx_command cmd_echo =
{
	.next = 0,
	.name = "echo",
	.func = cmd_echo_func,
	.help = "echo STRING ...\nDisplay message.",
};

/* exit */
static int cmd_exit_func(int argc, char** argv)
{
	winx_exit(0);
	return 0;
}

static struct winx_command cmd_exit =
{
	.next = 0,
	.name = "exit",
	.func = cmd_exit_func,
	.help = "exit\nExit native shell.",
};

/* reboot */
static int cmd_reboot_func(int argc, char** argv)
{
	winx_printf("Reboot ...\n");
	winx_reboot();
	winx_printf("\nReboot your computer manually.\n");
	return 0;
}

static struct winx_command cmd_reboot =
{
	.next = 0,
	.name = "reboot",
	.func = cmd_reboot_func,
	.help = "reboot\nReboot the computer.",
};

/* shutdown */
static int cmd_shutdown_func(int argc, char** argv)
{
	winx_printf("Shutdown ...\n");
	winx_shutdown();
	winx_printf("\nShutdown your computer manually.\n");
	return 0;
}

static struct winx_command cmd_shutdown =
{
	.next = 0,
	.name = "shutdown",
	.func = cmd_shutdown_func,
	.help = "shutdown\nShut the computer down.",
};

/* ls */
static int ls_filter(winx_file_info* f, void* data)
{
	return 0;
}

static void ls_progress(winx_file_info* f, void* data)
{
	if (is_directory(f))
		winx_printf(" [%S]", f->name);
	else
		winx_printf(" %S", f->name);
}

static int ls_terminator(void* data)
{
	return 0;
}

static int cmd_ls_func(int argc, char** argv)
{
	wchar_t* path;
	winx_file_info* list;

	if (argc < 2)
	{
		char vol;
		winx_volume_information info;
		char* suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB" };
		for (vol = 'A'; vol <= 'Z'; vol++)
		{
			if (winx_get_volume_information(vol, &info))
				continue;
			winx_printf("%c:\\ LABEL=[%S] FS=%s SIZE=%s\n",
				vol, info.label, info.fs_name, winx_get_human_size(info.total_bytes, suffixes, 1024));
		}
		return 0;
	}
	path = winx_swprintf(L"\\??\\%S", argv[1]);
	if (!path)
	{
		winx_printf("error invalid path\n");
		return (-1);
	}

	list = winx_ftw(path, 0, ls_filter, ls_progress, ls_terminator, NULL);
	winx_printf("\n");
	winx_ftw_release(list);
	winx_free(path);
	
	return 0;
}

static struct winx_command cmd_ls =
{
	.next = 0,
	.name = "ls",
	.func = cmd_ls_func,
	.help = "ls PATH\nList files.",
};

/* call */
static int cmd_call_func(int argc, char** argv)
{
	char* script = NULL;
	if (argc > 1)
		script = argv[1];
	return naoh_script(script);
}

static struct winx_command cmd_call =
{
	.next = 0,
	.name = "call",
	.func = cmd_call_func,
	.help = "call [SCRIPT]\nExecute a boot time script.",
};

/* exec */
static int cmd_exec_func(int argc, char** argv)
{
	int i;
	wchar_t* file = NULL;
	wchar_t* cmdline = NULL;
	if (argc < 2)
		return 0;
	file = winx_swprintf(L"\\??\\%S", argv[1]);
	for (i = 1; i < argc; i++)
	{
		wchar_t* tmp = NULL;
		if (cmdline)
		{
			tmp = winx_swprintf(L"%s %S", cmdline, argv[i]);
			winx_free(cmdline);
		}
		else
			tmp = winx_swprintf(L"%S", argv[i]);
		cmdline = tmp;
	}
	winx_printf("cmdline: %S\n", cmdline);
	winx_execute_native(file, cmdline);
	winx_free(cmdline);
	return 0;
}

static struct winx_command cmd_exec =
{
	.next = 0,
	.name = "exec",
	.func = cmd_exec_func,
	.help = "exec FILE [CMDLINE] ...\nExecute a native program.",
};

/* md */
static int cmd_md_func(int argc, char** argv)
{
	int rc;
	wchar_t* path = NULL;
	if (argc < 2)
		return 0;
	path = winx_swprintf(L"\\??\\%S", argv[1]);
	rc = winx_create_directory(path);
	winx_free(path);
	return rc;
}

static struct winx_command cmd_md =
{
	.next = 0,
	.name = "md",
	.func = cmd_md_func,
	.help = "md DIR\nCreate a directory.",
};

/* del */
static int cmd_del_func(int argc, char** argv)
{
	int rc;
	wchar_t* path = NULL;
	if (argc < 2)
		return 0;
	path = winx_swprintf(L"\\??\\%S", argv[1]);
	rc = winx_delete_file(path);
	winx_free(path);
	return rc;
}

static struct winx_command cmd_del =
{
	.next = 0,
	.name = "del",
	.func = cmd_del_func,
	.help = "del FILE\nDelete a file.",
};

/* mount */
static int cmd_mount_func(int argc, char** argv)
{
	int i, status;
	wchar_t letter = 0;
	wchar_t* path = NULL;
	if (argc < 2)
		return 0;
	for (i = 0; i < argc; i++)
	{
		if (strncmp(argv[i], "-d=", 3) == 0)
			letter = argv[i][3];
		else
			path = winx_swprintf(L"\\??\\%S", argv[i]);
	}
	if (!path)
		return -1;
	status = naoh_imdisk_mount(path, letter);
	winx_free(path);
	return status;
}

static struct winx_command cmd_mount =
{
	.next = 0,
	.name = "mount",
	.func = cmd_mount_func,
	.help = "mount [-d=X] FILE\nMount ISO|IMG file.",
};

void
naoh_cmd_init(void)
{
	winx_command_register(&cmd_shutdown);
	winx_command_register(&cmd_reboot);
	winx_command_register(&cmd_exit);
	winx_command_register(&cmd_mount);
	winx_command_register(&cmd_del);
	winx_command_register(&cmd_md);
	winx_command_register(&cmd_ls);
	winx_command_register(&cmd_echo);
	winx_command_register(&cmd_exec);
	winx_command_register(&cmd_call);
	winx_command_register(&cmd_help);
}
