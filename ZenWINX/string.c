/*
 *  ZenWINX - WIndows Native eXtended library.
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

/**
 * @file string.c
 * @brief Strings.
 * @addtogroup Strings
 * @{
 */

#include "prec.h"
#include "zenwinx.h"
#include <math.h> /* for pow function */

/**
 * @internal
 * @brief The initial buffer size for the 
 * winx_vsprintf and winx_vswprintf routines.
 * Increase this value to format long strings
 * faster.
 */
#define WINX_VSPRINTF_BUFFER_SIZE 128

/* tables for character case conversion */
#include "case-tables.h"

#define fast_toupper(c)  (ascii_uppercase[(int)(unsigned char)(c)])
#define fast_tolower(c)  (ascii_lowercase[(int)(unsigned char)(c)])
#define fast_towupper(c) (u16_uppercase[(unsigned int)(c)])
#define fast_towlower(c) (u16_lowercase[(unsigned int)(c)])

/**
 * @internal
 * @brief Initializes tables for
 * the character case conversion.
 */
void winx_init_case_tables(void)
{
    int i;
    
    for(i = 0; i < sizeof(u16_uppercase) / sizeof(wchar_t); i++){
        if(u16_uppercase[i] == 0) u16_uppercase[i] = (wchar_t)i;
        if(u16_lowercase[i] == 0) u16_lowercase[i] = (wchar_t)i;
    }
}

/**
 * @brief Reliable _toupper analog.
 * @details MSDN states: "In order for toupper to give
 * the expected results, __isascii and islower must both
 * return nonzero". winx_toupper has no such limitations.
 * @note Converts ASCII characters only (as well as the
 * _toupper function included in ntdll library).
 */
char winx_toupper(char c)
{
    return fast_toupper(c);
}

/**
 * @brief Reliable _tolower analog.
 * @details MSDN states: "In order for tolower to give
 * the expected results, __isascii and isupper must both
 * return nonzero". winx_tolower has no such limitations.
 * @note Converts ASCII characters only (as well as the
 * _tolower function included in ntdll library).
 */
char winx_tolower(char c)
{
    return fast_tolower(c);
}

/**
 * @brief Reliable towupper analog.
 * @details This routine doesn't depend
 * on the current locale, it converts
 * all the characters according to the
 * Unicode standard.
 */
wchar_t winx_towupper(wchar_t c)
{
    return fast_towupper(c);
}

/**
 * @brief Reliable towlower analog.
 * @details This routine doesn't depend
 * on the current locale, it converts
 * all the characters according to the
 * Unicode standard.
 */
wchar_t winx_towlower(wchar_t c)
{
    return fast_towlower(c);
}

/**
 * @brief Reliable _wcsupr analog.
 * @details This routine doesn't depend
 * on the current locale, it converts
 * all the characters according to the
 * Unicode standard.
 */
wchar_t *winx_wcsupr(wchar_t *s)
{
    wchar_t *cp;
    
    if(s){
        for(cp = s; *cp; cp++)
            *cp = fast_towupper(*cp);
    }
    return s;
}

/**
 * @brief Reliable _wcslwr analog.
 * @details This routine doesn't depend
 * on the current locale, it converts
 * all the characters according to the
 * Unicode standard.
 */
wchar_t *winx_wcslwr(wchar_t *s)
{
    wchar_t *cp;
    
    if(s){
        for(cp = s; *cp; cp++)
            *cp = fast_towlower(*cp);
    }
    return s;
}

/**
 * @brief strdup equivalent.
 */
char *winx_strdup(const char *s)
{
    int length;
    char *cp;
    
    if(s == NULL)
        return NULL;
    
    length = (int)strlen(s);
    cp = winx_tmalloc((length + 1) * sizeof(char));
    if(cp) strcpy(cp,s);
    return cp;
}

/**
 * @brief wcsdup equivalent.
 */
wchar_t *winx_wcsdup(const wchar_t *s)
{
    int length;
    wchar_t *cp;
    
    if(s == NULL)
        return NULL;
    
    length = (int)wcslen(s);
    cp = winx_tmalloc((length + 1) * sizeof(wchar_t));
    if(cp) wcscpy(cp,s);
    return cp;
}

/**
 * @brief Case insensitive version of wcscmp.
 * @details This routine doesn't depend
 * on the current locale, it converts
 * all the characters according to the
 * Unicode standard.
 */
int winx_wcsicmp(const wchar_t *s1, const wchar_t *s2)
{
    int result = 0;
    
    if(s1 == NULL || s2 == NULL)
        return (!s1 && !s2) ? 0 : 1;
    
    do {
        result = (int)(fast_towlower(*s1) - fast_towlower(*s2));
        if(result != 0) break;
        if(*s1 == 0) break;
        s1++, s2++;
    } while(1);

    return result;
}

/**
 * @brief Case insensitive version of wcsstr.
 * @details This routine doesn't depend
 * on the current locale, it converts
 * all the characters according to the
 * Unicode standard.
 */
wchar_t *winx_wcsistr(const wchar_t *s1, const wchar_t *s2)
{
    wchar_t *cp = (wchar_t *)s1;
    wchar_t *_s1, *_s2;

    if(s1 == NULL || s2 == NULL) return NULL;
    
    while(*cp){
        _s1 = cp;
        _s2 = (wchar_t *)s2;
        
        while(*_s1 && *_s2 && !( fast_towlower(*_s1) \
            - fast_towlower(*_s2) )){ _s1++, _s2++; }
        if(!*_s2) return cp;
        cp++;
    }
    
    return NULL;
}

/**
 * @brief Case insensitive version of strstr.
 * @note Compares case insensitively ASCII characters only.
 */
char *winx_stristr(const char *s1, const char *s2)
{
    char *cp = (char *)s1;
    char *_s1, *_s2;

    if(s1 == NULL || s2 == NULL) return NULL;
    
    while(*cp){
        _s1 = cp;
        _s2 = (char *)s2;
        
        while(*_s1 && *_s2 && !( fast_tolower(*_s1) \
            - fast_tolower(*_s2) )){ _s1++, _s2++; }
        if(!*_s2) return cp;
        cp++;
    }
    
    return NULL;
}

/**
 * @internal
 * @brief winx_wcsmatch helper.
 */
static int wcsmatch_helper(wchar_t *string, wchar_t *mask)
{
    wchar_t cs, cm;
    
    while(*string && *mask){
        cs = *string; cm = *mask;
        if(cs != cm && cm != '?'){
            /* the current pair of characters differs */
            if(cm != '*') return 0;
            /* skip asterisks */
            while(*mask == '*') mask ++;
            if(*mask == 0) return 1;
            /* compare the rest of the string with the rest of the mask */
            cm = *mask;
            if(cm == '?'){
                /* the question mark matches any single character */
                for(; *string; string++){
                    if(wcsmatch_helper(string, mask))
                        return 1;
                }
            } else {
                /* skip part of the string which doesn't match for sure */
                for(; *string; string++){
                    if(*string == cm){
                        if(wcsmatch_helper(string, mask))
                            return 1;
                    }
                }
            }
            return 0;
        }
        /* let's compare the next pair of characters */
        string ++;
        mask ++;
    }
    
    while(*mask == '*') mask ++;
    return (*string == 0 && *mask == 0) ? 1 : 0;
}

/**
 * @internal
 * @brief winx_wcsmatch helper.
 */
static int wcsmatch_icase_helper(wchar_t *string, wchar_t *mask)
{
    wchar_t cs, cm;
    
    while(*string && *mask){
        cs = fast_towlower(*string);
        cm = fast_towlower(*mask);
        if(cs != cm && cm != '?'){
            /* the current pair of characters differs */
            if(cm != '*') return 0;
            /* skip asterisks */
            while(*mask == '*') mask ++;
            if(*mask == 0) return 1;
            /* compare the rest of the string with the rest of the mask */
            cm = fast_towlower(*mask);
            if(cm == '?'){
                /* the question mark matches any single character */
                for(; *string; string++){
                    if(wcsmatch_icase_helper(string, mask))
                        return 1;
                }
            } else {
                /* skip part of the string which doesn't match for sure */
                for(; *string; string++){
                    if(fast_towlower(*string) == cm){
                        if(wcsmatch_icase_helper(string, mask))
                            return 1;
                    }
                }
            }
            return 0;
        }
        /* let's compare the next pair of characters */
        string ++;
        mask ++;
    }
    
    while(*mask == '*') mask ++;
    return (*string == 0 && *mask == 0) ? 1 : 0;
}

/**
 * @brief Compares a string with a mask.
 * @details Supports both <b>?</b> and <b>*</b> wildcards.
 * @param[in] string the string to be compared with the mask.
 * @param[in] mask the mask to be compared with the string.
 * @param[in] flags a combination of WINX_PAT_xxx flags.
 * @return A nonzero value if the string matches the mask.
 * @note Optimized for speed.
 */
int winx_wcsmatch(wchar_t *string, wchar_t *mask, int flags)
{
    if(string == NULL || mask == NULL)
        return 0;
    
    if(wcscmp(mask,L"*") == 0)
        return 1;
    
    if(flags & WINX_PAT_ICASE)
        return wcsmatch_icase_helper(string,mask);
    return wcsmatch_helper(string,mask);
}

/**
 * @brief A robust and flexible alternative to _vsnprintf.
 * @param[in] format the format specification.
 * @param[in] arg pointer to the list of arguments.
 * @return Pointer to the formatted string, NULL
 * indicates failure. The string should be deallocated
 * by winx_free after its use.
 * @note Optimized for speed, can allocate more memory than needed.
 */
char *winx_vsprintf(const char *format,va_list arg)
{
    char *buffer;
    int size;
    int result;
    
    /*
    * Avoid winx_dbg_xxx calls here
    * to avoid recursion.
    */
    
    if(format == NULL)
        return NULL;
    
    /* set the initial buffer size */
    size = WINX_VSPRINTF_BUFFER_SIZE;
    do {
        buffer = winx_tmalloc(size);
        if(!buffer) break;
        memset(buffer,0,size); /* needed for _vsnprintf */
        result = _vsnprintf(buffer,size,format,arg);
        if(result != -1 && result != size)
            return buffer;
        /* the buffer is too small; try to allocate two times larger */
        winx_free(buffer);
        size <<= 1;
    } while(size > 0);
    
    return NULL;
}

/**
 * @brief A robust and flexible alternative to _snprintf.
 * @param[in] format the format specification.
 * @param[in] ... the arguments.
 * @return Pointer to the formatted string, NULL
 * indicates failure. The string should be deallocated
 * by winx_free after its use.
 * @note Optimized for speed, can allocate more memory than needed.
 */
char *winx_sprintf(const char *format, ...)
{
    va_list arg;
    
    if(format){
        va_start(arg,format);
        return winx_vsprintf(format,arg);
    }
    
    return NULL;
}

/**
 * @brief A robust and flexible alternative to _vsnwprintf.
 * @param[in] format the format specification.
 * @param[in] arg pointer to the list of arguments.
 * @return Pointer to the formatted string, NULL
 * indicates failure. The string should be deallocated
 * by winx_free after its use.
 * @note Optimized for speed, can allocate more memory than needed.
 */
wchar_t *winx_vswprintf(const wchar_t *format,va_list arg)
{
    wchar_t *buffer;
    int size;
    int result;
    
    if(format == NULL)
        return NULL;
    
    /* set the initial buffer size */
    size = WINX_VSPRINTF_BUFFER_SIZE;
    do {
        buffer = winx_tmalloc(size * sizeof(wchar_t));
        if(!buffer) break;
        memset(buffer,0,size * sizeof(wchar_t)); /* needed for _vsnwprintf */
        result = _vsnwprintf(buffer,size,format,arg);
        if(result != -1 && result != size)
            return buffer;
        /* the buffer is too small; try to allocate two times larger */
        winx_free(buffer);
        size <<= 1;
    } while(size > 0);
    
    return NULL;
}

/**
 * @brief A robust and flexible alternative to _snwprintf.
 * @param[in] format the format specification.
 * @param[in] ... the arguments.
 * @return Pointer to the formatted string, NULL
 * indicates failure. The string should be deallocated
 * by winx_free after its use.
 * @note Optimized for speed, can allocate more memory than needed.
 */
wchar_t *winx_swprintf(const wchar_t *format, ...)
{
    va_list arg;
    
    if(format){
        va_start(arg,format);
        return winx_vswprintf(format,arg);
    }
    
    return NULL;
}

/*
* A lightweight alternative for regular expressions.
*/

/**
 * @brief Compiles a string of patterns
 * to a single winx_patlist structure.
 * @param[out] patterns pointer to a single
 * winx_patlist structure receiving the result.
 * @param[in] string the string of patterns.
 * @param[in] delim the list of delimiters to be
 * used to split the string to individual patterns.
 * @param[in] flags a combination of WINX_PAT_xxx flags.
 * @return Zero for success, a negative value otherwise.
 */
int winx_patcomp(winx_patlist *patterns,wchar_t *string,wchar_t *delim,int flags)
{
    int pattern_detected;
    int i, j, n;
    wchar_t *s;
    
    if(patterns == NULL || string == NULL || delim == NULL)
        return (-1);
    
    /* reset patterns structure */
    patterns->flags = flags;
    patterns->count = 0;
    patterns->array = NULL;
    patterns->string = NULL;
    
    if(string[0] == 0)
        return 0; /* empty list of patterns */
    
    /* make a copy of the string */
    s = winx_wcsdup(string);
    if(s == NULL){
        etrace("cannot allocate %u bytes of memory",
            (wcslen(string) + 1) * sizeof(wchar_t));
        return (-1);
    }
    
    /* replace all delimiters by zeros */
    for(n = 0; s[n]; n++){
        if(wcschr(delim,s[n]))
            s[n] = 0;
    }
    
    /* count all patterns */
    pattern_detected = 0;
    for(i = 0; i < n; i++){
        if(s[i] != 0){
            if(!pattern_detected){
                patterns->count ++;
                pattern_detected = 1;
            }
        } else {
            pattern_detected = 0;
        }
    }
    
    /* build array of patterns */
    patterns->array = winx_malloc(patterns->count * sizeof(wchar_t *));
    pattern_detected = 0;
    for(i = j = 0; i < n; i++){
        if(s[i] != 0){
            if(!pattern_detected){
                patterns->array[j] = s + i;
                j ++;
                pattern_detected = 1;
            }
        } else {
            pattern_detected = 0;
        }
    }

    patterns->string = s;
    return 0;
}

/**
 * @brief Searches for patterns in a string.
 * @param[in] string the string to search in.
 * @param[in] patterns the list of patterns
 * to be searched for.
 * @return A nonzero value if at least one
 * pattern has been found.
 */
int winx_patfind(wchar_t *string,winx_patlist *patterns)
{
    int i;
    wchar_t *result;
    
    if(patterns == NULL || string == NULL)
        return 0;
    
    for(i = 0; i < patterns->count; i++){
        if(patterns->flags & WINX_PAT_ICASE)
            result = winx_wcsistr(string,patterns->array[i]);
        else
            result = wcsstr(string,patterns->array[i]);
        if(result)
            return 1; /* pattern found */
    }
    /* no patterns found */
    return 0;
}

/**
 * @brief Compares a string with patterns.
 * @details Supports both <b>?</b> and <b>*</b> wildcards.
 * @param[in] string the string to compare patterns with.
 * @param[in] patterns the list of patterns
 * to be compared with the string.
 * @return A nonzero value if at least
 * one pattern matches the string.
 */
int winx_patcmp(wchar_t *string,winx_patlist *patterns)
{
    int i;
    
    if(string == NULL || patterns == NULL)
        return 0;
    
    for(i = 0; i < patterns->count; i++){
        if(winx_wcsmatch(string, patterns->array[i], patterns->flags))
            return 1;
    }
    /* no pattern matches the string */
    return 0;
}

/**
 * @brief Releases resources allocated by winx_patcomp.
 * @param[in] patterns the list of patterns.
 */
void winx_patfree(winx_patlist *patterns)
{
    if(patterns == NULL)
        return;
    
    /* free allocated memory */
    winx_free(patterns->string);
    winx_free(patterns->array);
    
    /* reset all fields of the structure */
    patterns->flags = 0;
    patterns->count = 0;
    patterns->array = NULL;
    patterns->string = NULL;
}

/*
* End of the lightweight alternative for regular expressions.
*/

const char*
winx_get_human_size(unsigned long long size, const char* human_sizes[6], unsigned long long base)
{
    unsigned long long fsize = size, frac = 0;
    unsigned units = 0;
    static char buf[48];
    const char* umsg;

    while (fsize >= base && units < 5)
    {
        frac = fsize % base;
        fsize = fsize / base;
        units++;
    }

    umsg = human_sizes[units];

    if (units)
    {
        if (frac)
            frac = frac * 100 / base;
        _snprintf(buf, sizeof(buf), "%llu.%02llu%s", fsize, frac, umsg);
    }
    else
        _snprintf(buf, sizeof(buf), "%llu%s", size, umsg);
    return buf;
}

/**
 * @brief Converts a string to UTF-8 encoding.
 * @param[out] dst the destination buffer.
 * @param[in] size size of the destination buffer.
 * @param[in] src the source string.
 * @note Each converted character requires maximum
 * three bytes to be stored. So, the destination buffer
 * should be at least (1.5 * src_bytes) long.
 */
void winx_to_utf8(char *dst,int size,wchar_t *src)
{
    int i; /* src index */
    int j; /* dst index */
    wchar_t c, b1, b2, b3;
    
    if(!src || !dst || size <= 0) return;
    
    for(i = j = 0; src[i]; i++){
        c = src[i];
        if(c < 0x80){
            if(j > (size - 2)) break;
            dst[j] = (char)c;
            j ++;
        } else if(c < 0x800){ /* 0x80 - 0x7FF: 2 bytes */
            if(j > (size - 3)) break;
            b2 = 0x80 | (c & 0x3F);
            c >>= 6;
            b1 = 0xC0 | c;
            dst[j] = (char)b1;
            dst[j+1] = (char)b2;
            j += 2;
        } else { /* 0x800 - 0xFFFF: 3 bytes */
            if(j > (size - 4)) break;
            b3 = 0x80 | (c & 0x3F);
            c >>= 6;
            b2 = 0x80 | (c & 0x3F);
            c >>= 6;
            b1 = 0xE0 | c;
            dst[j] = (char)b1;
            dst[j+1] = (char)b2;
            dst[j+2] = (char)b3;
            j += 3;
        }
    }
    dst[j] = 0;
}

/** @} */
