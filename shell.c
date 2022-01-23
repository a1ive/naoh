
#include "naoh.h"

void naoh_shell(PEB* peb)
{
	char buffer[MAX_LINE_WIDTH + 1];
	wchar_t wbuffer[MAX_LINE_WIDTH + 1];
	winx_history history = { 0 };

	winx_init_history(&history);
	while(winx_prompt("# ", buffer, MAX_LINE_WIDTH, &history) >= 0)
	{
		if(_snwprintf_s(wbuffer, MAX_LINE_WIDTH, MAX_LINE_WIDTH, L"%hs", buffer) < 0)
		{
			winx_printf("Command line is too long!\n");
			continue;
		}
		wbuffer[MAX_LINE_WIDTH] = 0;

		winx_parse_command(&history, wbuffer, peb);
	}
	winx_destroy_history(&history);
	winx_exit(0);
}
