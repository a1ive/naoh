#include "naoh.h"
#include "imdisk.h"

static wchar_t FindFreeDriveLetter(VOID)
{
	wchar_t Letter;
	wchar_t Path[] = L"\\??\\A:";
	WINX_FILE* hDrive;

	for (Letter = L'C'; Letter <= L'Z'; Letter++)
	{
		Path[4] = Letter;
		hDrive = winx_fopen(Path, "r");
		if (!hDrive)
			return Letter;
		winx_fclose(hDrive);
	}
	return L']';
}

int
ImdiskMount(PCWSTR FileName,
	LARGE_INTEGER Offest,
	LARGE_INTEGER Length,
	MEDIA_TYPE MediaType,
	WCHAR DriveLetter)
{
	PIMDISK_CREATE_DATA ImdiskData;
	size_t ImdiskDataSize;
	WINX_FILE* hImdisk;
	int Status;

	hImdisk = winx_fopen(L"\\??\\ImDiskCtl", "r+");
	if (!hImdisk)
	{
		winx_printf("Imdisk driver open failed.\n");
		return -1;
	}

	ImdiskDataSize = sizeof(IMDISK_CREATE_DATA) + (wcslen(FileName) + 1) * sizeof(WCHAR);
	ImdiskData = (PIMDISK_CREATE_DATA)winx_malloc(ImdiskDataSize);
	ImdiskData->DeviceNumber = IMDISK_AUTO_DEVICE_NUMBER;
	ImdiskData->DiskGeometry.MediaType = MediaType;

	if (MediaType == RemovableMedia)
	{
		ImdiskData->DiskGeometry.BytesPerSector = 2048;
		ImdiskData->Flags = IMDISK_OPTION_RO | IMDISK_TYPE_FILE | IMDISK_DEVICE_TYPE_CD;
	}
	else
	{
		ImdiskData->Flags = IMDISK_TYPE_FILE | IMDISK_DEVICE_TYPE_HD;
		ImdiskData->DiskGeometry.MediaType = FixedMedia;
		ImdiskData->DiskGeometry.BytesPerSector = 512;
		ImdiskData->DiskGeometry.SectorsPerTrack = 63;
		ImdiskData->DiskGeometry.TracksPerCylinder = 255;
	}
	ImdiskData->DiskGeometry.Cylinders = Length;
	ImdiskData->ImageOffset = Offest;
	ImdiskData->DriveLetter = ((DriveLetter == 0) ? (FindFreeDriveLetter()) : DriveLetter);
	ImdiskData->FileNameLength = (USHORT)(wcslen(FileName) * sizeof(WCHAR));
	memcpy(ImdiskData->FileName, FileName, ImdiskData->FileNameLength);

	Status = winx_ioctl(hImdisk, IOCTL_IMDISK_CREATE_DEVICE, NULL, ImdiskData, (int)ImdiskDataSize, NULL, 0, NULL);
	winx_fclose(hImdisk);
	winx_free(ImdiskData);
	return Status;
}

int naoh_imdisk_mount(wchar_t* file_name, wchar_t drive_letter)
{
	WINX_FILE* file;
	LARGE_INTEGER offset, length;
	size_t name_len = wcslen(file_name);
	MEDIA_TYPE type = FixedMedia;
	file = winx_fopen(file_name, "r");
	if (!file)
	{
		winx_printf("file open failed\n");
		return -1;
	}
	offset.QuadPart = 0;
	length.QuadPart = winx_fsize(file);
	winx_fclose(file);
	if (name_len > 4 && _wcsnicmp(L".iso", &file_name[name_len - 4], 4) == 0)
		type = RemovableMedia;
	return ImdiskMount(file_name, offset, length, type, drive_letter);
}


