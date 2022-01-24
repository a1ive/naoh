#include "naoh.h"

int naoh_script(const char *filename)
{
	wchar_t* path = NULL;
	char* buffer;
	size_t filesize, i;
	int line_detected;
	KBD_RECORD kbd_rec;

	/* read the script file entirely */
	if(filename == NULL)
	{
		wchar_t* windir = winx_get_windows_directory();
		if(windir == NULL)
		{
			winx_printf("\ncannot get %%windir%% path\n\n");
			return (-1);
		}
		path = winx_swprintf(L"%ws\\system32\\naoh.cmd",windir);
		winx_free(windir);
	}
	else
	{
		path = winx_swprintf(L"\\??\\%S",filename);
	}

	if (!path)
		return -1;

	buffer = winx_get_file_contents(path, &filesize);
	winx_free(path);
	if(buffer == NULL || filesize == 0)
		return 0;

	/* terminate the buffer */
	buffer[filesize] = 0;
	
	/* replace all the newline characters by zeros */
	for(i = 0; i < filesize; i++)
	{
		if(buffer[i] == '\n' || buffer[i] == '\r')
			buffer[i] = 0;
	}
	/* skip UTF-8 BOM */
	if (buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
		memset(buffer, 0, 3);

	/* parse lines */
	line_detected = 0;
	for(i = 0; i < filesize; i++)
	{
		if(buffer[i] != 0)
		{
			if(!line_detected)
			{
				winx_command_parse(buffer + i);
				line_detected = 1;
				if(winx_kb_read(&kbd_rec,0) == 0)
				{
					if(kbd_rec.wVirtualScanCode == 0x1)
						goto cleanup;
				}
			}
		}
		else
			line_detected = 0;
	}

cleanup:
	winx_release_file_contents(buffer);
	return 0;
}
