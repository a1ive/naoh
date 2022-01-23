
#include "naoh.h"

void __stdcall NtProcessStartup(PPEB Peb)
{
	if (winx_init_library() < 0)
	{
		winx_print("ZenWINX library init failed!\n");
		goto fail;
	}
	winx_print("ZenWINX library init OK.\n");
	if (winx_windows_in_safe_mode())
	{
		winx_print("SafeMode detected!\n");
		goto fail;
	}
	if (winx_kb_init() < 0)
	{
		winx_print("ZenWINX keyboard init failed!\n");
		goto fail;
	}
	winx_print("ZenWINX keyboard init OK.\n");
	winx_print("Press any key ...\n");
	winx_kbhit(INFINITE);
	naoh_shell(Peb);
fail:
	winx_exit(0);
}
