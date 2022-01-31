#ifndef _NAOH_H_
#define _NAOH_H_

#include "ZenWINX/ntndk.h"
#include "ZenWINX/zenwinx.h"
#include "ZenWINX/commands.h"
#include "ZenWINX/charset.h"

void naoh_shell(void);
int naoh_script(const char* filename);
void naoh_cmd_init(void);

int naoh_imdisk_mount(wchar_t* file_name, wchar_t drive_letter);;

#endif
