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
#include <stdint.h>

#include "myprintf.h"
/* If we have a project config file include it. */
#ifdef USE_PROJECT_CONFIG_H
#include "project_config.h"
#endif

/* Do you want binary format? Watch out it needs a big buffer. */
#ifndef CFG_MYPRINTF_WANT_BINARY
#define CFG_MYPRINTF_WANT_BINARY 0
#endif

/* Customisation for AVR target. */
#if defined(__AVR__)

/* If MYPRINTF_WANT_PGM_STR defined then this macro used to access characters in PROGMEM. */
#include <avr/pgmspace.h>
#define MYPRINTF_DEREF_PGM_STR_CHAR(ptr_) ((char)pgm_read_byte(ptr_))		// For AVR.

/* Define if strings can be in Flash as well as RAM. This enables the `%S' format. */
#define MYPRINTF_WANT_PGM_STR

#else

#define MYPRINTF_DEREF_PGM_STR_CHAR(ptr_) (*(ptr_))
#define PROGMEM /*empty */
#undef MYPRINTF_WANT_PGM_STR

#endif

/* Check that we have size of myprintf's int type correct. */
#define MYPRINTF_STATIC_ASSERT(expr_) extern char error_static_assert_fail__[(expr_) ? 1 : -1] __attribute__((unused))
#define IS_TYPE_SIGNED(T_) (((T_)(-1)) < (T_)0)
MYPRINTF_STATIC_ASSERT(IS_TYPE_SIGNED(CFG_MYPRINTF_TYPE_SIGNED_INT));
MYPRINTF_STATIC_ASSERT(!IS_TYPE_SIGNED(CFG_MYPRINTF_TYPE_UNSIGNED_INT));
MYPRINTF_STATIC_ASSERT(IS_TYPE_SIGNED(CFG_MYPRINTF_TYPE_SIGNED_LONG_INT));
MYPRINTF_STATIC_ASSERT(!IS_TYPE_SIGNED(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT));
MYPRINTF_STATIC_ASSERT(sizeof(CFG_MYPRINTF_TYPE_SIGNED_INT) == sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_INT));
MYPRINTF_STATIC_ASSERT(sizeof(CFG_MYPRINTF_TYPE_SIGNED_LONG_INT) == sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT));

MYPRINTF_STATIC_ASSERT(sizeof(CFG_MYPRINTF_TYPE_SIGNED_INT) < sizeof(CFG_MYPRINTF_TYPE_SIGNED_LONG_INT));

// Maximum value
#define MYPRINTF_MAX_UNSIGNED_LONG_INT ((CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT)(-1))

/* How many digits required for the maximum value of an unsigned long in your smallest base?  */
MYPRINTF_STATIC_ASSERT(
 (sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT) == 2) ||
 (sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT) == 4) ||
 (sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT) == 8));
#if CFG_MYPRINTF_WANT_BINARY
 #define MYPRINTF_MAX_DIGIT_CHARS (sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT) * 8)
#else
  #define MYPRINTF_MAX_DIGIT_CHARS (5 * sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT) / 2)
#endif

// If the preprocessor excludes code snippets these warnings can go off, so disable them.
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wunused-label"

struct snprintf_state {
	char* p;	/* Next char pos to be written to buffer. */
	char* end;	/* Next char after last char in buffer. Don't write here. */
};

static void myprintf_snprintf_of(char c, void* arg) {
	struct snprintf_state* s = (struct snprintf_state*)arg;
	if (s->p < s->end)	/* Will write entire buffer but no more. */
		*s->p++ = c;
}
char myprintf_vsnprintf(char* buf, unsigned len, const char* fmt, va_list ap) {
	char rc = 0;			// Default is fail.
	if (len > 0) {			// Require room for at least a nul.
		struct snprintf_state s = { buf, buf + len };
		myprintf(myprintf_snprintf_of, (void*)(&s), fmt, ap);
		if (s.p >= s.end) { /* Overflow! */
			s.p[-1] = '\0';
		}
		else {				/* No overflow. */
			s.p[0] = '\0';
			rc = 1;
		}
	}
	return rc;
}

char myprintf_snprintf(char* buf, unsigned len, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	char rc = myprintf_vsnprintf(buf, len, fmt, ap);
	va_end(ap);
	return rc;
}

/* This has to be a macro, some weirdness with va_args. It expands to not much machine code.
 * You can pass them as arguments to functions and call va_arg() on them, but not in two separate functions it seems. 
 * Anyway I had a failure on AVR with a format like "%ld %d", added a test, found the same error on the x86 run test.
 * Then fixed the fault. */
#define grab_integer() (																\
  ((flags & FLAG_LONG) && (sizeof(CFG_MYPRINTF_TYPE_SIGNED_LONG_INT) > sizeof(int))) ?	\
    va_arg(ap, CFG_MYPRINTF_TYPE_SIGNED_LONG_INT) :										\
	va_arg(ap, int)																		\
  )

void myprintf(myprintf_putchar putfunc, void* arg, const char* fmt, va_list ap) {
	uint_least8_t width;			// Holds field width.
	uint_least8_t base;				// Holds number base.
	char padchar;					// Holds padding char, either space or '0'.

	enum {
		FLAG_FORMAT = 1, FLAG_LONG = 2, FLAG_PAD_RIGHT = 4, FLAG_UPPER = 8, FLAG_NEG = 16, FLAG_STR_PGM = 32,
	};
	uint_least8_t flags = 0;		// Various flags.

	union {
		CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT u;
		CFG_MYPRINTF_TYPE_SIGNED_LONG_INT i;
	} num;							// Number to print. Always positive.

	/* the following should be enough for 32 bit int */
	enum { BUF_LEN = MYPRINTF_MAX_DIGIT_CHARS + 1 + 1 };  // Sized for max no. of digits + a possible '-' and a nul.
	char buf[BUF_LEN];
	union {
		const char* c;
		char* m;
	} str;
	char c;

	for (; '\0' != (c = MYPRINTF_DEREF_PGM_STR_CHAR(fmt)); fmt += 1) {	// Iterate over all chars in format.
		if (flags & FLAG_FORMAT) {
			if ('-' == c) {			// A '-' for right justification.
				c = MYPRINTF_DEREF_PGM_STR_CHAR(++fmt);
				flags |= FLAG_PAD_RIGHT;
			}
			else if ('0' == c) {			// A leading zero for zero pad.
				c = MYPRINTF_DEREF_PGM_STR_CHAR(++fmt);
				padchar = '0';
			}
			while ((c >= '0') && (c <= '9')) {	// Read width value.
				width *= 10;
				width += c - '0';
				c = MYPRINTF_DEREF_PGM_STR_CHAR(++fmt);
			}
			if (('l' == c) || ('L' == c)) {			// A 'l' or 'L' for long integer.
				c = MYPRINTF_DEREF_PGM_STR_CHAR(++fmt);
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
					static const char NULL_STR[] PROGMEM = "(null)";
					str.c = NULL_STR;
#ifdef MYPRINTF_WANT_PGM_STR
					flags |= FLAG_STR_PGM;				/* Set PROGMEM, this is a no-op for non-AVRs. */
#endif // MYPRINTF_WANT_PGM_STR
				}
				goto p_str;
			case 'c':
				buf[0] = (char)va_arg(ap, int);			/* Gcc can give a warning about char promotion when using va_arg(). */
				buf[1] = '\0';
				str.c = &buf[0];
				goto p_str;
			case 'd':
				num.i = (CFG_MYPRINTF_TYPE_SIGNED_LONG_INT)grab_integer();
				if (num.i < 0) {
					num.i = -num.i;
					flags |= FLAG_NEG;
				}
				break;
#if CFG_MYPRINTF_WANT_BINARY
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
do_unsigned:	num.u = (CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT)grab_integer(); // cppcheck-suppress unusedLabelSwitchConfiguration
				break;
			case '%': 						/* Literal '%', just print it. */
				flags = 0;					/* Clear format flag. */
				goto out;
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
			if (width > 0) { 	// Get length of stringonly if required.
				int len;
				const char* p = str.c;
				while ('\0' != (flags & FLAG_STR_PGM) ? MYPRINTF_DEREF_PGM_STR_CHAR(p) : *p)
					p += 1;
				len = p - str.c;

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

			while ('\0' != (c = (flags & FLAG_STR_PGM) ? MYPRINTF_DEREF_PGM_STR_CHAR(str.c) : *str.c)) {
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
