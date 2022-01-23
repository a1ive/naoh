
#include "naoh.h"

void naoh_shell(void)
{
	char buffer[MAX_LINE_WIDTH + 1];
	winx_history history = { 0 };

	winx_init_history(&history);
	while(winx_prompt("# ", buffer, MAX_LINE_WIDTH, &history) >= 0)
	{
		winx_command_parse(buffer);
	}
	winx_destroy_history(&history);
	winx_exit(0);
}
