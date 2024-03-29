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
 * @file stdio.c
 * @brief Console I/O.
 * @addtogroup Console
 * @{
 */

#include "prec.h"
#include "zenwinx.h"

#define INTERNAL_BUFFER_SIZE 2048

int kb_read(PKEYBOARD_INPUT_DATA pKID,int msec_timeout);
void IntTranslateKey(PKEYBOARD_INPUT_DATA InputData, KBD_RECORD *kbd_rec);

/**
 * @brief Displays an ANSI string on the screen.
 * @details This routine requires no library
 * initialization, thus may be used when it fails.
 * @param[in] string the string to be displayed.
 * @note Doesn't recognize the <b>\\b</b> character.
 */
void winx_print(char *string)
{
    ANSI_STRING as;
    UNICODE_STRING us;
    int i;
    
    /*
    * Use neither memory allocation nor debugging
    * routines: they may be not available here.
    */

    if(!string)
        return;
    
    /* use slower winx_putch based code if \t detected */
    if(strchr(string,'\t'))
        goto second_algorithm;

    RtlInitAnsiString(&as,string);
    if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) == STATUS_SUCCESS){
        NtDisplayString(&us);
        RtlFreeUnicodeString(&us);
    } else {
second_algorithm:
        for(i = 0; string[i]; i ++)
            winx_putch(string[i]);
    }
}

/**
 * @brief putch() native equivalent.
 * @note Doesn't recognize the <b>\\b</b> character.
 */
int winx_putch(int ch)
{
    UNICODE_STRING us;
    int i;
    wchar_t t[DEFAULT_TAB_WIDTH + 1];
    wchar_t s[2];

    /*
    * Use neither memory allocation nor debugging
    * routines: they may be not available here.
    */
    
    if(ch == '\t'){
        for(i = 0; i < DEFAULT_TAB_WIDTH; i++) t[i] = 0x20;
        t[DEFAULT_TAB_WIDTH] = 0;
        RtlInitUnicodeString(&us,t);
    } else {
        s[0] = (wchar_t)ch; s[1] = 0;
        RtlInitUnicodeString(&us,s);
    }
    NtDisplayString(&us);
    return ch;
}

/**
 * @brief puts() native equivalent.
 * @note Doesn't recognize the <b>\\b</b> character.
 */
int winx_puts(const char *string)
{
    if(!string) return (-1);
    return winx_printf("%s\n",string) ? 0 : (-1);
}

/**
 * @brief printf() native equivalent.
 * @note Doesn't recognize the <b>\\b</b> character.
 */
int winx_printf(const char *format, ...)
{
    va_list arg;
    char *string;
    int result = 0;
    
    if(format){
        va_start(arg,format);
        string = winx_vsprintf(format,arg);
        if(string){
            winx_print(string);
            result = (int)strlen(string);
            winx_free(string);
        }
        va_end(arg);
    }
    
    return result;
}

/**
 * @brief Checks the console for keyboard input.
 * @details Waits for input during the specified time interval.
 * @param[in] msec the timeout interval, in milliseconds.
 * @return Whenever the user hits any key this function
 * returns its corresponding ASCII character (zero for
 * control keys). Negative values indicate failure.
 * @note If the INFINITE time constant is passed, 
 * the timeout interval never elapses.
 */
int winx_kbhit(int msec)
{
    KBD_RECORD kbd_rec;

    return winx_kb_read(&kbd_rec,msec);
}

/**
 * @brief A low level winx_kbhit() equivalent.
 * @param[in] kbd_rec pointer to structure
 * receiving the key information.
 * @param[in] msec the timeout interval, in milliseconds.
 * @return Whenever the user hits any key this function
 * returns its corresponding ASCII character (zero for
 * control keys). Negative values indicate failure.
 * @note If the INFINITE time constant is passed, 
 * the timeout interval never elapses.
 */
int winx_kb_read(KBD_RECORD *kbd_rec,int msec)
{
    KEYBOARD_INPUT_DATA kbd;
    
    DbgCheck1(kbd_rec,-1);

    do{
        if(kb_read(&kbd,msec) < 0) return (-1);
        IntTranslateKey(&kbd,kbd_rec);
    } while(!kbd_rec->bKeyDown); /* skip key up events */
    return (int)kbd_rec->AsciiChar;
}

/**
 * @brief Waits for the Break key hits
 * during the specified time interval.
 * @param[in] msec the timeout interval, in milliseconds.
 * @return Whenever the user hits the Break key this function
 * returns zero. Otherwise it returns a negative value.
 * @note If the INFINITE time constant is passed, 
 * the timeout interval never elapses.
 */
int winx_breakhit(int msec)
{
    KEYBOARD_INPUT_DATA kbd;
    KBD_RECORD kbd_rec;

    do{
        if(kb_read(&kbd,msec) < 0) return (-1);
        IntTranslateKey(&kbd,&kbd_rec);
    } while(!kbd_rec.bKeyDown); /* skip key up events */
    if((kbd.Flags & KEY_E1) && (kbd.MakeCode == 0x1d)) return 0;
    /*winx_printf("\nwinx_breakhit(): Other key was pressed.\n");*/
    return (-1);
}

/**
 * @brief getch() native equivalent.
 */
int winx_getch(void)
{
    KBD_RECORD kbd_rec;
    
    return winx_kb_read(&kbd_rec,INFINITE);
}

/**
 * @brief getche() native equivalent.
 * @note 
 * - Doesn't recognize the <b>\\b</b> character.
 * - Doesn't recognize tabulation.
 */
int winx_getche(void)
{
    int ch;

    ch = winx_getch();
    if(ch != -1 && ch != 0 && ch != 0x08) /* skip backspace */
        winx_putch(ch);
    return ch;
}

/**
 * @brief gets() native equivalent with
 * limited number of characters to read.
 * @param[out] string the output buffer.
 * @param[in] n size of the buffer, in characters.
 * @return Number of characters read including the
 * terminal zero. Negative values indicate failure.
 * @note Doesn't recognize tabulation.
 */
int winx_gets(char *string,int n)
{
    return winx_prompt(NULL,string,n,NULL);
}

/**
 * @brief Initializes commands history.
 * @param[in] h pointer to commands history.
 */
void winx_init_history(winx_history *h)
{
    if(h == NULL){
        etrace("h = NULL!");
        return;
    }
    h->head = h->current = NULL;
    h->n_entries = 0;
}

/**
 * @brief Destroys commands history.
 * @param[in] h pointer to commands history.
 */
void winx_destroy_history(winx_history *h)
{
    winx_history_entry *entry;
    
    if(h == NULL){
        etrace("h = NULL!");
        return;
    }

    for(entry = h->head; entry != NULL; entry = entry->next){
        winx_free(entry->string);
        if(entry->next == h->head) break;
    }
    winx_list_destroy((list_entry **)(void *)&h->head);
    h->current = NULL;
    h->n_entries = 0;
}

/**
 * @internal
 * @brief Adds an entry to the commands history list.
 */
static void winx_add_history_entry(winx_history *h,char *string)
{
    winx_history_entry *entry, *last_entry = NULL;
    int length;
    
    if(h == NULL || string == NULL) return;
    
    if(h->head) last_entry = h->head->prev;
    entry = (winx_history_entry *)winx_list_insert((list_entry **)(void *)&h->head,
        (list_entry *)last_entry,sizeof(winx_history_entry));
    
    entry->string = winx_strdup(string);
    if(entry->string == NULL){
        length = (int)strlen(string) + 1;
        etrace("cannot allocate %u bytes of memory",length);
        winx_printf("\nCannot allocate %u bytes of memory for %s()!\n",length,__FUNCTION__);
        winx_list_remove((list_entry **)(void *)&h->head,(list_entry *)entry);
    } else {
        h->n_entries ++;
        h->current = entry;
    }
}

/**
 * @brief Displays a prompt on the screen and waits for
 * user input. Whenever the user hits the return key it
 * fills the output buffer by characters read.
 * @param[in] prompt the prompt to be displayed.
 * @param[out] string the output buffer.
 * @param[in] n size of the buffer, in characters.
 * @param[in,out] h pointer to commands history. May be NULL.
 * @return Number of characters read, including the terminal
 * zero. Negative values indicate failure.
 * @note
 * - Recognizes properly both backslash and escape keys, but
 * only when the input takes just a single line on the screen.
 * - Recognizes arrow keys to walk through the history of commands.
 * - Doesn't recognize tabulation.
 */
int winx_prompt(char *prompt,char *string,int n,winx_history *h)
{
    KEYBOARD_INPUT_DATA kbd;
    KBD_RECORD kbd_rec;
    char *buffer;
    char format[16];
    int i, ch, line_length, buffer_length;
    int history_listed_to_the_last_entry = 0;

    if(!string){
        winx_printf("\nwinx_prompt: invalid string!\n");
        return (-1);
    }
    if(n <= 0){
        winx_printf("\nwinx_prompt: invalid string length %d!\n",n);
        return (-1);
    }
    
    if(!prompt) prompt = "";
    buffer_length = (int)strlen(prompt) + n;
    buffer = winx_malloc(buffer_length);
    
    winx_printf("%s",prompt);
    
    /* keep the string always null-terminated */
    RtlZeroMemory(string,n);
    for(i = 0; i < (n - 1); i ++){
        /* read keyboards until an ordinary character appearance */
        do {
            do{
                if(kb_read(&kbd,INFINITE) < 0) goto fail;
                IntTranslateKey(&kbd,&kbd_rec);
            } while(!kbd_rec.bKeyDown);
            ch = (int)kbd_rec.AsciiChar;
            /*
            * Truncate the string if either backspace or escape pressed.
            * Walk through the history if one of the arrow keys pressed.
            */
            if(ch == 0x08 || kbd_rec.wVirtualScanCode == 0x1 || \
              kbd_rec.wVirtualScanCode == 0x48 || kbd_rec.wVirtualScanCode == 0x50){
                line_length = (int)strlen(prompt) + (int)strlen(string);
                /* handle escape hits */
                if(kbd_rec.wVirtualScanCode == 0x1){
                    /* truncate the string */
                    RtlZeroMemory(string,n);
                    i = 0;
                }
                /* handle backspace hits */
                if(ch == 0x08){
                    /* make the string one character shorter */
                    if(i > 0){
                        i--;
                        string[i] = 0;
                    }
                }
                /* handle arrow keys */
                if(h){
                    if(h->head && h->current){
                        if(kbd_rec.wVirtualScanCode == 0x48){
                            /* list history back */
                            if(h->current == h->head->prev && \
                              !history_listed_to_the_last_entry){
                                /* set the flag and don't list back */
                                history_listed_to_the_last_entry = 1;
                            } else {
                                if(h->current != h->head)
                                    h->current = h->current->prev;
                                history_listed_to_the_last_entry = 0;
                            }
                            if(h->current->string){
                                RtlZeroMemory(string,n);
                                strcpy(string,h->current->string);
                                i = (int)strlen(string);
                            }
                        } else if(kbd_rec.wVirtualScanCode == 0x50){
                            /* list history forward */
                            if(h->current->next != h->head){
                                h->current = h->current->next;
                                if(h->current->string){
                                    RtlZeroMemory(string,n);
                                    strcpy(string,h->current->string);
                                    i = (int)strlen(string);
                                }
                                if(h->current == h->head->prev)
                                    history_listed_to_the_last_entry = 1;
                                else
                                    history_listed_to_the_last_entry = 0;
                            }
                        }
                    }
                }
                
                /* clear the history_listed_to_the_last_entry flag */
                if(ch == 0x08 || kbd_rec.wVirtualScanCode == 0x1)
                    history_listed_to_the_last_entry = 0;
                
                /* redraw the prompt */
                _snprintf(buffer,buffer_length,"%s%s",prompt,string);
                buffer[buffer_length - 1] = 0;
                _snprintf(format,sizeof(format),"\r%%-%us",line_length);
                format[sizeof(format) - 1] = 0;
                winx_printf(format,buffer);
                /*
                * redraw the prompt once again to set carriage
                * position exactly behind the string printed
                */
                line_length = (int)strlen(prompt) + (int)strlen(string);
                _snprintf(format,sizeof(format),"\r%%-%us",line_length);
                format[sizeof(format) - 1] = 0;
                winx_printf(format,buffer);
                continue;
            }
        } while(ch == 0 || ch == 0x08 || kbd_rec.wVirtualScanCode == 0x1 || \
              kbd_rec.wVirtualScanCode == 0x48 || kbd_rec.wVirtualScanCode == 0x50);
        
        /* print the character read */
        winx_putch(ch);

        /* return when the \r character appears */
        if(ch == '\r'){
            winx_putch('\n');
            goto done;
        }
        
        /* we have an ordinary character, append it to the string */
        string[i] = (char)ch;
        
        /* clear the flag in case of ordinary characters typed */
        history_listed_to_the_last_entry = 0;
    }
    winx_printf("\nwinx_prompt: buffer overflow!\n");

done:
    winx_free(buffer);
    /* add nonempty strings to the history */
    if(string[0]){
        winx_add_history_entry(h,string);
    }
    return (i+1);
    
fail:
    winx_free(buffer);
    return (-1);
}

/* returns 1 if either break or escape have been pressed, zero otherwise */
static int print_line(char *line_buffer,
    char *prompt,int max_rows,int *rows_printed,
    int last_line)
{
    KBD_RECORD kbd_rec;
    int escape_detected = 0;
    int break_detected = 0;

    winx_printf("%s\n",line_buffer);
    (*rows_printed) ++;
    
    if(*rows_printed == max_rows && !last_line){
        *rows_printed = 0;
        winx_printf("\n%s\n",prompt);
        /* wait for any key */
        if(winx_kb_read(&kbd_rec,INFINITE) < 0){
            return 1; /* break in case of errors */
        }
        /* check for escape */
        if(kbd_rec.wVirtualScanCode == 0x1){
            escape_detected = 1;
        } else if(kbd_rec.wVirtualScanCode == 0x1d){
            /* distinguish between control keys and the break key */
            if(!(kbd_rec.dwControlKeyState & LEFT_CTRL_PRESSED) && \
              !(kbd_rec.dwControlKeyState & RIGHT_CTRL_PRESSED)){
                break_detected = 1;
            }
        }
        winx_printf("\n");
        if(escape_detected || break_detected) return 1;
    }
    return 0;
}

/**
 * @brief Displays an array of strings on the 
 * screen, divided to pages whenever requested.
 * @param[in] strings the array of strings
 * to be displayed. The last entry must be
 * NULL to indicate the end of the array.
 * @param[in] line_width the maximum
 * line width, in characters.
 * @param[in] max_rows the maximum
 * number of lines on the screen.
 * @param[in] prompt a string to be
 * displayed as the prompt to hit any
 * key to list forward.
 * @param[in] divide_to_pages defines
 * whether the text must be divided to
 * pages or not. If this parameter is
 * equal to zero, all others, except
 * the first one, will be ignored.
 * @return Zero for success, a negative
 * value otherwise.
 * @note
 * - If the user hits either Escape or
 * Pause the listing terminates immediately.
 * - Doesn't recognize the <b>\\b</b> character.
 */
int winx_print_strings(char **strings,int line_width,
    int max_rows,char *prompt,int divide_to_pages)
{
    int i, j, k, length, index;
    char *line_buffer, *second_buffer;
    int n, r;
    int rows_printed;
    
    /* validate the main parameter */
    DbgCheck1(strings, -1);
    
    /* handle the case when the text must be displayed entirely */
    if(!divide_to_pages){
        for(i = 0; strings[i] != NULL; i++)
            winx_printf("%s\n",strings[i]);
        return 0;
    }

    /* validate other parameters */
    if(!line_width){
        etrace("line_width = 0!");
        return (-1);
    }
    if(!max_rows){
        etrace("max_rows = 0!");
        return (-1);
    }
    if(prompt == NULL) prompt = DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY;
    
    /* allocate space for the prompt */
    max_rows -= 4;
    
    /* allocate memory */
    line_buffer = winx_malloc(line_width + 1);
    second_buffer = winx_malloc(line_width + 1);
    
    /* start to display strings */
    rows_printed = 0;
    for(i = 0; strings[i] != NULL; i++){
        line_buffer[0] = 0;
        index = 0;
        length = (int)strlen(strings[i]);
        for(j = 0; j < length; j++){
            /* handle \n, \r, \r\n, \n\r sequencies */
            n = r = 0;
            if(strings[i][j] == '\n') n = 1;
            else if(strings[i][j] == '\r') r = 1;
            if(n || r){
                /* print the buffer */
                line_buffer[index] = 0;
                if(print_line(line_buffer,prompt,max_rows,&rows_printed,0))
                    goto cleanup;
                /* reset the buffer */
                line_buffer[0] = 0;
                index = 0;
                /* skip the sequence */
                j++;
                if(j == length) goto print_rest_of_string;
                if((strings[i][j] == '\n' && r) || (strings[i][j] == '\r' && n)){
                    continue;
                } else {
                    if(strings[i][j] == '\n' || strings[i][j] == '\r'){
                        /* process repeating new lines */
                        j--;
                        continue;
                    }
                    /* we have an ordinary character or a tabulation -> process them */
                }
            }
            /* handle horizontal tabulations by replacing them by DEFAULT_TAB_WIDTH spaces */
            if(strings[i][j] == '\t'){
                for(k = 0; k < DEFAULT_TAB_WIDTH; k++){
                    line_buffer[index] = 0x20;
                    index ++;
                    if(index == line_width){
                        if(j == length - 1) goto print_rest_of_string;
                        line_buffer[index] = 0;
                        if(print_line(line_buffer,prompt,max_rows,&rows_printed,0))
                            goto cleanup;
                        line_buffer[0] = 0;
                        index = 0;
                        break;
                    }
                }
                continue;
            }
            /* handle ordinary characters */
            line_buffer[index] = strings[i][j];
            index ++;
            if(index == line_width){
                if(j == length - 1) goto print_rest_of_string;
                line_buffer[index] = 0;
                /* break the line between words, whenever possible */
                for(k = index - 1; k > 0; k--){
                    if(line_buffer[k] == 0x20) break;
                }
                if(line_buffer[k] == 0x20){ /* the space character found */
                    strcpy(second_buffer,line_buffer + k + 1);
                    line_buffer[k] = 0;
                    if(print_line(line_buffer,prompt,max_rows,&rows_printed,0))
                        goto cleanup;
                    strcpy(line_buffer,second_buffer);
                    index = (int)strlen(line_buffer);
                } else {
                    if(print_line(line_buffer,prompt,max_rows,&rows_printed,0))
                        goto cleanup;
                    line_buffer[0] = 0;
                    index = 0;
                }
            }
        }
print_rest_of_string:
        line_buffer[index] = 0;
        if(print_line(line_buffer,prompt,max_rows,&rows_printed,
            (strings[i+1] == NULL) ? 1 : 0)) goto cleanup;
    }

cleanup:
    winx_free(line_buffer);
    winx_free(second_buffer);
    return 0;
}

/** @} */
