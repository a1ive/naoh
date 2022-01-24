
#include "prec.h"
#include "ntndk.h"
#include "zenwinx.h"

static NTSTATUS CreateNativeProcess(IN PCWSTR file_name, IN PCWSTR cmd_line, OUT PHANDLE hProcess)
{
    UNICODE_STRING nt_file;
    PCWSTR file_part;
    NTSTATUS status; // Status
    UNICODE_STRING imgname; // ImageName
    UNICODE_STRING imgpath; // Nt ImagePath
    UNICODE_STRING dllpath; // Nt DllPath (DOS Name)
    UNICODE_STRING cmdline; // Nt CommandLine
    PRTL_USER_PROCESS_PARAMETERS processparameters; // ProcessParameters
    RTL_USER_PROCESS_INFORMATION processinformation = { 0 }; // ProcessInformation
    WCHAR Env[2] = { 0,0 }; // Process Envirnoment
    PKUSER_SHARED_DATA SharedData = (PKUSER_SHARED_DATA)USER_SHARED_DATA; // Kernel Shared Data

    *hProcess = NULL;

    RtlDosPathNameToNtPathName_U(file_name, &nt_file, &file_part, NULL);

    RtlInitUnicodeString(&imgpath, nt_file.Buffer); // Image path
    RtlInitUnicodeString(&imgname, file_part); // Image name
    RtlInitUnicodeString(&dllpath, SharedData->NtSystemRoot); // DLL Path is %SystemRoot%
    RtlInitUnicodeString(&cmdline, cmd_line); // Command Line parameters

    status = RtlCreateProcessParameters(&processparameters, &imgname, &dllpath, &dllpath, &cmdline, Env, 0, 0, 0, 0);

    if (!NT_SUCCESS(status))
    {
        winx_printf("RtlCreateProcessParameters failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    //DbgPrint("Launching Process: %s, DllPath=%s, CmdLine=%s", &imgname, &dllpath, &cmdline);
    status = RtlCreateUserProcess(&imgpath, OBJ_CASE_INSENSITIVE, processparameters,
        NULL, NULL, NULL, FALSE, NULL, NULL, &processinformation);

    if (!NT_SUCCESS(status))
    {
        winx_printf("RtlCreateUserProcess failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    status = NtResumeThread(processinformation.ThreadHandle, NULL);

    if (!NT_SUCCESS(status))
    {
        winx_printf("NtResumeThread failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    *hProcess = processinformation.ProcessHandle;

    return STATUS_SUCCESS;
}

void winx_execute_native(const wchar_t* file, const wchar_t* cmdline)
{
    UNICODE_STRING us;
    HANDLE hProcess;
    RtlInitUnicodeString(&us, cmdline);

    winx_kb_close();

    CreateNativeProcess(file, us.Buffer, &hProcess);

    RtlFreeUnicodeString(&us);

    NtWaitForSingleObject(hProcess, FALSE, NULL);

    winx_kb_open();
}
