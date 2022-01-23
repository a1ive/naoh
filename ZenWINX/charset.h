/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2022  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _CHARSET_HEADER
#define _CHARSET_HEADER 1

#include <stdint.h>

#define WINX_UINT8_1_LEADINGBIT 0x80
#define WINX_UINT8_2_LEADINGBITS 0xc0
#define WINX_UINT8_3_LEADINGBITS 0xe0
#define WINX_UINT8_4_LEADINGBITS 0xf0
#define WINX_UINT8_5_LEADINGBITS 0xf8
#define WINX_UINT8_6_LEADINGBITS 0xfc
#define WINX_UINT8_7_LEADINGBITS 0xfe

#define WINX_UINT8_1_TRAILINGBIT 0x01
#define WINX_UINT8_2_TRAILINGBITS 0x03
#define WINX_UINT8_3_TRAILINGBITS 0x07
#define WINX_UINT8_4_TRAILINGBITS 0x0f
#define WINX_UINT8_5_TRAILINGBITS 0x1f
#define WINX_UINT8_6_TRAILINGBITS 0x3f

#define WINX_MAX_UTF8_PER_UTF16 4
 /* You need at least one UTF-8 byte to have one UTF-16 word.
	You need at least three UTF-8 bytes to have 2 UTF-16 words (surrogate pairs).
  */
#define WINX_MAX_UTF16_PER_UTF8 1
#define WINX_MAX_UTF8_PER_CODEPOINT 4

#define WINX_UCS2_LIMIT 0x10000
#define WINX_UTF16_UPPER_SURROGATE(code) \
  (0xD800 | ((((code) - WINX_UCS2_LIMIT) >> 10) & 0x3ff))
#define WINX_UTF16_LOWER_SURROGATE(code) \
  (0xDC00 | (((code) - WINX_UCS2_LIMIT) & 0x3ff))

  /* Process one character from UTF8 sequence.
	 At beginning set *code = 0, *count = 0. Returns 0 on failure and
	 1 on success. *count holds the number of trailing bytes.  */
static inline int
winx_utf8_process(uint8_t c, uint32_t* code, int* count)
{
	if (*count)
	{
		if ((c & WINX_UINT8_2_LEADINGBITS) != WINX_UINT8_1_LEADINGBIT)
		{
			*count = 0;
			/* invalid */
			return 0;
		}
		else
		{
			*code <<= 6;
			*code |= (c & WINX_UINT8_6_TRAILINGBITS);
			(*count)--;
			/* Overlong.  */
			if ((*count == 1 && *code <= 0x1f)
				|| (*count == 2 && *code <= 0xf))
			{
				*code = 0;
				*count = 0;
				return 0;
			}
			return 1;
		}
	}

	if ((c & WINX_UINT8_1_LEADINGBIT) == 0)
	{
		*code = c;
		return 1;
	}
	if ((c & WINX_UINT8_3_LEADINGBITS) == WINX_UINT8_2_LEADINGBITS)
	{
		*count = 1;
		*code = c & WINX_UINT8_5_TRAILINGBITS;
		/* Overlong */
		if (*code <= 1)
		{
			*count = 0;
			*code = 0;
			return 0;
		}
		return 1;
	}
	if ((c & WINX_UINT8_4_LEADINGBITS) == WINX_UINT8_3_LEADINGBITS)
	{
		*count = 2;
		*code = c & WINX_UINT8_4_TRAILINGBITS;
		return 1;
	}
	if ((c & WINX_UINT8_5_LEADINGBITS) == WINX_UINT8_4_LEADINGBITS)
	{
		*count = 3;
		*code = c & WINX_UINT8_3_TRAILINGBITS;
		return 1;
	}
	return 0;
}


/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UTF-16 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters. If an invalid sequence is found, return -1.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
static inline size_t
winx_utf8_to_utf16(uint16_t* dest, size_t destsize,
	const uint8_t* src, size_t srcsize,
	const uint8_t** srcend)
{
	uint16_t* p = dest;
	int count = 0;
	uint32_t code = 0;

	if (srcend)
		*srcend = src;

	while (srcsize && destsize)
	{
		int was_count = count;
		if (srcsize != (size_t)-1)
			srcsize--;
		if (!winx_utf8_process(*src++, &code, &count))
		{
			code = '?';
			count = 0;
			/* Character c may be valid, don't eat it.  */
			if (was_count)
				src--;
		}
		if (count != 0)
			continue;
		if (code == 0)
			break;
		if (destsize < 2 && code >= WINX_UCS2_LIMIT)
			break;
		if (code >= WINX_UCS2_LIMIT)
		{
			*p++ = WINX_UTF16_UPPER_SURROGATE(code);
			*p++ = WINX_UTF16_LOWER_SURROGATE(code);
			destsize -= 2;
		}
		else
		{
			*p++ = (uint16_t)code;
			destsize--;
		}
	}

	if (srcend)
		*srcend = src;
	return p - dest;
}

/* Determine the last position where the UTF-8 string [beg, end) can
   be safely cut. */
static inline size_t
winx_getend(const char* beg, const char* end)
{
	const char* ptr;
	for (ptr = end - 1; ptr >= beg; ptr--)
		if ((*ptr & WINX_UINT8_2_LEADINGBITS) != WINX_UINT8_1_LEADINGBIT)
			break;
	if (ptr < beg)
		return 0;
	if ((*ptr & WINX_UINT8_1_LEADINGBIT) == 0)
		return ptr + 1 - beg;
	if ((*ptr & WINX_UINT8_3_LEADINGBITS) == WINX_UINT8_2_LEADINGBITS
		&& ptr + 2 <= end)
		return ptr + 2 - beg;
	if ((*ptr & WINX_UINT8_4_LEADINGBITS) == WINX_UINT8_3_LEADINGBITS
		&& ptr + 3 <= end)
		return ptr + 3 - beg;
	if ((*ptr & WINX_UINT8_5_LEADINGBITS) == WINX_UINT8_4_LEADINGBITS
		&& ptr + 4 <= end)
		return ptr + 4 - beg;
	/* Invalid character or incomplete. Cut before it.  */
	return ptr - beg;
}

/* Convert UTF-16 to UTF-8.  */
static inline uint8_t*
winx_utf16_to_utf8(uint8_t* dest, const uint16_t* src, size_t size)
{
	uint32_t code_high = 0;

	while (size--)
	{
		uint32_t code = *src++;

		if (code_high)
		{
			if (code >= 0xDC00 && code <= 0xDFFF)
			{
				/* Surrogate pair.  */
				code = ((code_high - 0xD800) << 10) + (code - 0xDC00) + 0x10000;

				*dest++ = (uint8_t)((code >> 18) | 0xF0);
				*dest++ = (uint8_t)(((code >> 12) & 0x3F) | 0x80);
				*dest++ = (uint8_t)(((code >> 6) & 0x3F) | 0x80);
				*dest++ = (uint8_t)((code & 0x3F) | 0x80);
			}
			else
			{
				/* Error...  */
				*dest++ = '?';
				/* *src may be valid. Don't eat it.  */
				src--;
			}

			code_high = 0;
		}
		else
		{
			if (code <= 0x007F)
				*dest++ = (uint8_t)code;
			else if (code <= 0x07FF)
			{
				*dest++ = (uint8_t)((code >> 6) | 0xC0);
				*dest++ = (uint8_t)((code & 0x3F) | 0x80);
			}
			else if (code >= 0xD800 && code <= 0xDBFF)
			{
				code_high = code;
				continue;
			}
			else if (code >= 0xDC00 && code <= 0xDFFF)
			{
				/* Error... */
				*dest++ = '?';
			}
			else if (code < 0x10000)
			{
				*dest++ = (uint8_t)((code >> 12) | 0xE0);
				*dest++ = (uint8_t)(((code >> 6) & 0x3F) | 0x80);
				*dest++ = (uint8_t)((code & 0x3F) | 0x80);
			}
			else
			{
				*dest++ = (uint8_t)((code >> 18) | 0xF0);
				*dest++ = (uint8_t)(((code >> 12) & 0x3F) | 0x80);
				*dest++ = (uint8_t)(((code >> 6) & 0x3F) | 0x80);
				*dest++ = (uint8_t)((code & 0x3F) | 0x80);
			}
		}
	}

	return dest;
}

#endif
