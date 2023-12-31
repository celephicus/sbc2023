/*
	Copyright 2001-2021 Georges Menie
	https://www.menie.org/georges/embedded/small_printf_source_code.html

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdarg.h>
#include <stdlib.h>

#include "myprintf.h"
#include "myprintf.local.h"

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wunused-label"

#ifndef MYPRINTF_WANT_PGM_STR 
#define myprintf_deref_pgm_str_char(ptr_) (*(ptr_))		
#define myprintf_deref_fmt_char(ptr_) (*(ptr_))		
#endif

static void myprintf_sprintf_of(char c, void* arg) {
	char** s = (char**)arg;
	**s = c;
	*s += 1;
}

void myprintf_sprintf(char* buf, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	myprintf(myprintf_sprintf_of, (void*)(&buf), fmt, ap);
	*buf = '\0';
	va_end(ap);
}


void myprintf(myprintf_putchar putfunc, void* arg, const char* fmt, va_list ap) {
	myprintf_u8_t width;			// Holds field width.
	myprintf_u8_t base;				// Holds number base. 
	char padchar;					// Holds padding char, either space or '0'. 

	enum {
		FLAG_FORMAT = 1, FLAG_LONG = 2, FLAG_PAD_RIGHT = 4, FLAG_UPPER = 8, FLAG_NEG = 16, FLAG_STR_PGM = 32,
	};
	myprintf_u8_t flags = 0;		// Various flags.

	union {
		unsigned MYPRINTF_LONG_QUALIFIER u;
		MYPRINTF_LONG_QUALIFIER i;
	} num;							// Number to print. Always positive.

	/* the following should be enough for 32 bit int */
	enum { BUF_LEN = MYPRINTF_MAX_DIGIT_CHARS + 1 + 1 };  // Sized for max no. of digits + a possible '-' and a nul.
	char buf[BUF_LEN];
	union {
		const char* c;
		char* m;
	} str;
	char c;

	for (; '\0' != (c = myprintf_deref_fmt_char(fmt)); fmt += 1) {	// Iterate over all chars in format.
		if (flags & FLAG_FORMAT) {
			if ('%' == c) {		/* Literal '%', just print it. */
				flags = 0;				/* Clear format flag. */
				goto out;
			}
			if ('-' == c) {			// A '-' for right justification.
				c = myprintf_deref_fmt_char(++fmt);
				flags |= FLAG_PAD_RIGHT;
			}
			if ('0' == c) {			// A leading zero for zero pad. 
				c = myprintf_deref_fmt_char(++fmt);
				padchar = '0';
			}
			while ((c >= '0') && (c <= '9')) {	// Read width value. 
				width *= 10;
				width += c - '0';
				c = myprintf_deref_fmt_char(++fmt);
			}
			if (('l' == c) || ('L' == c)) {			// A 'l' or 'L' for long integer.
				c = myprintf_deref_fmt_char(++fmt);
				flags |= FLAG_LONG;
			}
			
			switch (c) {								// Now we have format spec.
#ifdef MYPRINTF_WANT_PGM_STR
			case 'S':									// String in program space.
				flags |= FLAG_STR_PGM;
				// Fall through...
#endif // MYPRINTF_WANT_PGM_STR
			case 's':
				str.c = va_arg(ap, const char*);
				if (NULL == str.c) {					/* Catch null ptr. */
					buf[0] = '\0';
#ifdef MYPRINTF_WANT_PGM_STR
					flags &= ~FLAG_STR_PGM;;			/* Might have been set by `%s' so clear. */
#endif // MYPRINTF_WANT_PGM_STR
					goto p_char;						/* Will be treated as zero length string. */
				}
#if 0
				flags &= ~FLAG_PAD_RIGHT;				/* Only pad strings with a space. */
#endif
				goto p_str;
			case 'c':
				buf[0] = (char)va_arg(ap, int);			/* Gcc can give a warning about char promotion when using va_arg(). */
p_char:			buf[1] = '\0';
				str.c = &buf[0];
#if 0
				flags &= ~FLAG_PAD_RIGHT;				/* Only pad strings with a space. */
#endif
				goto p_str;
			case 'd':
				num.i = ((flags & FLAG_LONG) ? va_arg(ap, MYPRINTF_LONG_QUALIFIER int) : (MYPRINTF_LONG_QUALIFIER int)va_arg(ap, int));
				if (num.i < 0) {
					num.i = -num.i;
					flags |= FLAG_NEG;
				}
				break;
#if defined(MYPRINTF_WANT_BINARY)
			case 'b':
				base = 2;
				goto do_unsigned;
#endif				
			case 'X':
				flags |= FLAG_UPPER;
				// Fall through...
			case 'x':
				base = 16;
				// Fall through...
			case 'u':
do_unsigned:	num.u = ((flags & FLAG_LONG) ? va_arg(ap, unsigned MYPRINTF_LONG_QUALIFIER int) : (unsigned MYPRINTF_LONG_QUALIFIER int)va_arg(ap, unsigned));
				break;
			default:										/* Unknown format! */
				flags = 0;				/* Clear format flag. */
				continue;
			}  // Closes ' switch (c) { ... '

			// Print number `num' justified in `width' with padding char. 
			str.m = buf + BUF_LEN - 1;			// Start building number from far end.
			*str.m = '\0';

			// Print digits LSD first. 
			do {
				const uint_least8_t digit = num.u % base;
				*--str.m = digit + ((digit < 10) ? '0' : (((flags & FLAG_UPPER) ? 'A' : 'a') - 10));
				num.u /= base;
			} while (num.u > 0);

			// Take care of leading '-'. 
			if (flags & FLAG_NEG) {
				if ((width > 0) && ('0' == padchar)) {
					putfunc('-', arg);
					width -= 1;
				}
				else
					*--str.m = '-';
			}

p_str:		// Print string `str' justified in `width' with padding char `pad'. 

			// Get length of string. Could replace with strlen()
			if (width > 0) {
				register int len;
				register const char* p;
				for (p = str.c, len = 0; '\0' != (flags & FLAG_STR_PGM) ? myprintf_deref_pgm_str_char(p) : *p; p += 1)
					len += 1;

					// Get length of padding required.
				if (len >= width)
					width = 0;
				else
					width -= len;
			}

			// Print padding to left.
			if (!(flags & FLAG_PAD_RIGHT)) {
				while (width > 0) {
					putfunc(padchar, arg);
					width -= 1;				// Can't use width-- in loop test as need width untouched later. 
				}
			}

			// Print string.
			
			while ('\0' != (c = (flags & FLAG_STR_PGM) ? myprintf_deref_pgm_str_char(str.c) : *str.c)) {
				putfunc(c, arg);
				str.c += 1;
			}

			// Print padding to right.
			while (width-- > 0)
				putfunc(padchar, arg);

			flags = 0;				// Clear in format flag. 
		}	// Closes `if (flags & FLAG_FORMAT) { ... '
		else {
			if ('%' == c) {				// We will be expecting format stuff now.
				width = 0;
				flags = FLAG_FORMAT;
				base = 10;
				padchar = ' ';
			}
			else {
out:		
				putfunc(c, arg);
			} // Closes 'if (*fmt == '%') { ... '
		}
	}	// Closes 	'for (; *fmt != 0; fmt += 1) { ... '
}
