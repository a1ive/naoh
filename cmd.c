
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

void
naoh_cmd_init(void)
{
	winx_command_register(&cmd_shutdown);
	winx_command_register(&cmd_reboot);
	winx_command_register(&cmd_exit);
	winx_command_register(&cmd_echo);
	winx_command_register(&cmd_help);
}
