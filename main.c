
#include "ZenWINX/ntndk.h"
#include "ZenWINX/zenwinx.h"

PEB* peb = NULL;

void __stdcall NtProcessStartup(PPEB Peb)
{
	peb = Peb;
	if (winx_init_library() < 0)
	{
		winx_print("ZenWINX library init failed!\n");
		goto fail;
	}
	winx_print("ZenWINX library init OK.\n");
	if (winx_kb_init() < 0)
	{
		winx_print("ZenWINX keyboard init failed!\n");
		goto fail;
	}
	winx_print("ZenWINX keyboard init OK.\n");
	winx_print("Press any key ...\n");
	winx_kbhit(INFINITE);
fail:
	winx_exit(0);
}
