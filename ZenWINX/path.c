/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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

/**
 * @file path.c
 * @brief Paths.
 * @addtogroup Paths
 * @{
 */

#include "prec.h"
#include "zenwinx.h"

/**
 * @brief Removes file extension from a path.
 * @param[in,out] path the path to be processed.
 * @note Filenames starting with a dot remain
 * untouched.
 */
void winx_path_remove_extension(wchar_t *path)
{
    int i;

    if(!path) return;
    
    for(i = (int)wcslen(path) - 1; i >= 0; i--){
        if(path[i] == '\\')
            return; /* filename contains no dot */
        if(path[i] == '.' && i){
            if(path[i - 1] != '\\')
                path[i] = 0;
            return;
        }
    }
}

/**
 * @brief Removes filename from a path.
 * @param[in,out] path the path to be processed.
 * @par Examples:
@verbatim
\??\C:\Windows\Test.txt -> \??\C:\Windows
\??\C:\Program Files\   -> \??\C:\Program Files
@endverbatim
 */
void winx_path_remove_filename(wchar_t *path)
{
    wchar_t *lb;
    
    if(path){
        lb = wcsrchr(path,'\\');
        if(lb) *lb = 0;
    }
}

/**
 * @brief Extracts filename from a path.
 * @param[in,out] path the path to be processed.
 * @par Examples:
@verbatim
\??\C:\Windows\Test.txt -> Test.txt
\??\C:\Program Files\   -> Program Files
@endverbatim
 */
void winx_path_extract_filename(wchar_t *path)
{
    int i,j,n;
    
    if(!path) return;
    
    n = (int)wcslen(path);
    if(!n) return;
    
    /* remove trailing backslash */
    if(path[n - 1] == '\\'){
        path[n - 1] = 0; n--;
        if(!n) return;
    }
    
    for(i = n - 1; i >= 0; i--){
        if(path[i] == '\\'){
            /* path[i+1] points to filename */
            i++;
            for(j = 0; path[i]; i++, j++)
                path[j] = path[i];
            path[j] = 0;
            return;
        }
    }
}

/**
 * @brief Gets the fully quallified path of the current module.
 * @details This routine is the native equivalent of the GetModuleFileName routine.
 * @note The returned string should be released by the winx_free call after its use.
 */
wchar_t *winx_get_module_filename(void)
{
    PROCESS_BASIC_INFORMATION pi;
    NTSTATUS status;
    UNICODE_STRING *us;
    wchar_t *path;
    int size;
    
    RtlZeroMemory(&pi,sizeof(pi));
    status = NtQueryInformationProcess(NtCurrentProcess(),
                    ProcessBasicInformation,&pi,
                    sizeof(pi),NULL);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot query basic process information");
        return NULL;
    }
    
    us = &pi.PebBaseAddress->ProcessParameters->ImagePathName;
    size = us->MaximumLength + sizeof(wchar_t);
    path = winx_tmalloc(size);
    if(path == NULL){
        mtrace();
        return NULL;
    }
    
    memset(path,0,size);
    memcpy(path,us->Buffer,us->Length);
    
    /*
    * At Windows boot '\??\' sequence gets
    * prepended to the path. Let's remove it.
    */
    if(wcsstr(path,L"\\??\\") == path){
        memset(path,0,size);
        memcpy(path,us->Buffer + 4,
            us->Length - 4 * sizeof(wchar_t)
        );
    }

    return path;
}

/**
 * @brief Creates a directory tree.
 * @param[in] path the native path to be created.
 * @return Zero for success, a negative value otherwise.
 */
int winx_create_path(wchar_t *path)
{
    /*wchar_t rootdir[] = L"\\??\\X:\\";*/
    winx_volume_information v;
    wchar_t *p;
    size_t n;
    
    if(path == NULL)
        return (-1);

    /* the path must contain at least \??\X: */
    if(wcsstr(path,L"\\??\\") != path || wcschr(path,':') != (path + 5)){
        etrace("native path must be specified");
        return (-1);
    }

    n = wcslen(L"\\??\\X:\\");
    if(wcslen(path) <= n){
        /* check for the volume existence */
        /*
        rootdir[4] = path[4];
        // may fail with access denied status
        return winx_create_directory(rootdir);
        */
        return winx_get_volume_information((char)path[4],&v);
    }
    
    /* skip \??\X:\ */
    p = path + n;
    
    /* create directory tree */
    while((p = wcschr(p,'\\'))){
        *p = 0;
        if(winx_create_directory(path) < 0){
            *p = '\\';
            etrace("cannot create %ws",path);
            return (-1);
        }
        *p = '\\';
        p ++;
    }
    
    /* create target directory */
    if(winx_create_directory(path) < 0){
        etrace("cannot create %ws",path);
        return (-1);
    }
    
    return 0;
}

/** @} */
