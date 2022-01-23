/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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
* UltraDefrag boot time (native) interface - boot time script parsing code.
*/

#include "prec.h"
#include "zenwinx.h"
#include "commands.h"

/**
 * @brief Processes the boot time script.
 * @param[in] filename the full path of the script.
 * If NULL is passed, the default boot time script
 * will be used.
 * @return Zero for success, a negative value otherwise.
 */
int winx_process_script(winx_history* history, wchar_t *filename, PEB* peb)
{
	wchar_t *windir;
	wchar_t path[MAX_PATH + 1];
	wchar_t *buffer;
	size_t filesize, i, n;
	int line_detected;
	KBD_RECORD kbd_rec;

	/* read the script file entirely */
	if(filename == NULL)
	{
		windir = winx_get_windows_directory();
		if(windir == NULL)
		{
			winx_printf("\nProcessScript: cannot get %%windir%% path\n\n");
			return (-1);
		}
		(void)_snwprintf(path,MAX_PATH,L"%ws\\system32\\naoh.cmd",windir);
		path[MAX_PATH] = 0;
		winx_free(windir);
	}
	else
	{
		(void)_snwprintf(path,MAX_PATH,L"\\??\\%ws",filename);
		path[MAX_PATH] = 0;
	}

	buffer = winx_get_file_contents(path,&filesize);
	if(buffer == NULL)
		return 0; /* the file is empty or some error occured */

	/* get file size, in characters */
	n = filesize / sizeof(wchar_t);
	if(n == 0)
		goto cleanup; /* the file has no valuable contents */

	/* terminate the buffer */
	buffer[n] = 0;
	
	/* replace all the newline characters by zeros */
	for(i = 0; i < n; i++)
	{
		if(buffer[i] == '\n' || buffer[i] == '\r')
			buffer[i] = 0;
	}
	/* skip the first 0xFEFF character added by Notepad */
	if(buffer[0] == 0xFEFF)
		buffer[0] = 0;

	/* parse lines */
	line_detected = 0;
	for(i = 0; i < n; i++)
	{
		if(buffer[i] != 0)
		{
			if(!line_detected)
			{
				(void)winx_parse_command(history, buffer + i, peb);
				line_detected = 1;
				if(winx_kb_read(&kbd_rec,0) == 0)
				{
					if(kbd_rec.wVirtualScanCode == 0x1)
						goto cleanup;
				}
			}
		}
		else
		{
			line_detected = 0;
		}
	}

cleanup:
	winx_release_file_contents(buffer);
	return 0;
}
