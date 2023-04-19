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

/* Do you want binary format? Watch out it needs a big buffer. */
#ifndef CFG_MYPRINTF_WANT_BINARY
#define CFG_MYPRINTF_WANT_BINARY 0
#endif

/* We need custom strings for the cursed AVR that live in a different address space "PGM", so we need macros to declare and access them. 
	If active the `%S' format prints a PGM string.
	Macro MYPRINTF_PGM_STR_DECL used to declare a string, e.g. `const char foo[] MYPRINTF_PGM_STR_DECL = "...";'
	Macro MYPRINTF_PGM_STR_DEREF_FMT(p) dereferences a const char* pointer FROM THE FORMAT to char.
	Macro MYPRINTF_WANT_PGM_STR set true if the PGM feature is active.

	Also includes option for testing.
	Note that arg is evaluated only once so is safe if used as MYPRINTF_PGM_STR_DEREF_FMT(p++).
*/

#if defined(__AVR__)
 #include <avr/pgmspace.h>
 #define MYPRINTF_PGM_STR_DEREF_FMT(ptr_) ((char)pgm_read_byte(ptr_))	
 #define MYPRINTF_PGM_STR_DEREF_PTR(ptr_) ( (flags & FLAG_STR_PGM) ? ((char)pgm_read_byte(ptr_)) : (*(ptr_))
 #define MYPRINTF_PGM_STR_DECL PROGMEM
 #define MYPRINTF_WANT_PGM_STR 1
#elif defined (TEST)
 #define MYPRINTF_PGM_STR_DEREF_FMT(ptr_) (*(ptr_))
 static char munge_for_testing(char c, bool f) { return (f && c) ? (c + 1) : c; }
 #define MYPRINTF_PGM_STR_DEREF_PTR(ptr_) (munge_for_testing(*(ptr_), flags & FLAG_STR_PGM))
 #define MYPRINTF_PGM_STR_DECL /*empty */
 #define MYPRINTF_WANT_PGM_STR 0
#else
 #define MYPRINTF_PGM_STR_DEREF_FMT(ptr_) (*(ptr_))
 #define MYPRINTF_PGM_STR_DEREF_PTR(ptr_) (*(ptr_))
 #define MYPRINTF_PGM_STR_DECL /*empty */
 #define MYPRINTF_WANT_PGM_STR 0
#endif

/* Check that we have size of myprintf's int type correct. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"

#define MYPRINTF_STATIC_ASSERT(expr_) extern char error_static_assert_fail__[(expr_) ? 1 : -1] __attribute__((unused))
#define IS_TYPE_SIGNED(T_) (((T_)(-1)) < (T_)0)
MYPRINTF_STATIC_ASSERT(IS_TYPE_SIGNED(CFG_MYPRINTF_T_INT));
MYPRINTF_STATIC_ASSERT(!IS_TYPE_SIGNED(CFG_MYPRINTF_T_UINT));
MYPRINTF_STATIC_ASSERT(IS_TYPE_SIGNED(CFG_MYPRINTF_T_L_INT));
MYPRINTF_STATIC_ASSERT(!IS_TYPE_SIGNED(CFG_MYPRINTF_T_L_UINT));
MYPRINTF_STATIC_ASSERT(sizeof(CFG_MYPRINTF_T_INT) == sizeof(CFG_MYPRINTF_T_UINT));
MYPRINTF_STATIC_ASSERT(sizeof(CFG_MYPRINTF_T_L_INT) == sizeof(CFG_MYPRINTF_T_L_UINT));

MYPRINTF_STATIC_ASSERT(sizeof(CFG_MYPRINTF_T_INT) < sizeof(CFG_MYPRINTF_T_L_INT));

/* How many digits required for the maximum value of an unsigned long in your smallest base?  */
MYPRINTF_STATIC_ASSERT(
 (sizeof(CFG_MYPRINTF_T_L_UINT) == 2) ||
 (sizeof(CFG_MYPRINTF_T_L_UINT) == 4) ||
 (sizeof(CFG_MYPRINTF_T_L_UINT) == 8) ||
 (sizeof(CFG_MYPRINTF_T_L_UINT) == 16));
#if CFG_MYPRINTF_WANT_BINARY
 #define MYPRINTF_MAX_DIGIT_CHARS (sizeof(CFG_MYPRINTF_T_L_UINT) * 8)
#else
  #define MYPRINTF_MAX_DIGIT_CHARS (5 * sizeof(CFG_MYPRINTF_T_L_UINT) / 2)
#endif

#pragma GCC diagnostic pop

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
char myprintf_vsnprintf(char* buf, size_t len, const char* fmt, va_list ap) {
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

char myprintf_snprintf(char* buf, size_t len, const char* fmt, ...) {
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
#define grab_integer(myprintf_type_, native_type_) (	\
  (sizeof(myprintf_type_) > sizeof(native_type_)) ?		\
    va_arg(ap, myprintf_type_) :						\
	(myprintf_type_)va_arg(ap, native_type_)			\
  )

void myprintf(myprintf_putchar putfunc, void* arg, const char* fmt, va_list ap) {
	uint_least8_t width;			// Holds field width.
	uint_least8_t base;				// Holds number base.
	char padchar;					// Holds padding char, either space or '0'.

	enum {
		FLAG_FORMAT = 1, 	// Parsing format,cleared on specifier char.
		FLAG_LONG = 2, 		// Integral type is LONG.
		FLAG_PAD_RIGHT = 4, // LEFT justify, pad to R with spaces. 
							// Else RIGHT justify, pad to L with spaces or zeroes.
		FLAG_UPPER = 8, 	// Upper case for hex (X vs x). 
		FLAG_NEG = 16, 		// Signed value negative.
		FLAG_STR_PGM = 32,	// String in PGM mamory (AVR only). 
	};
	uint_least8_t flags = 0;		// Various flags.

	union {
		CFG_MYPRINTF_T_L_UINT u;
		CFG_MYPRINTF_T_L_INT i;
	} num;							// Number to print. Always positive.

	/* the following should be enough for 32 bit int */
	enum { BUF_LEN = MYPRINTF_MAX_DIGIT_CHARS + 1 + 1 };  // Sized for max no. of digits + a possible '-' and a nul.
	char buf[BUF_LEN];
	union {
		const char* c;
		char* m;
	} str;
	char c;

	for (; '\0' != (c = MYPRINTF_PGM_STR_DEREF_FMT(fmt)); fmt += 1) {	// Iterate over all chars in format.
		if (flags & FLAG_FORMAT) {
			switch (c) {								// Now we have format spec.
			case '-':									// A '-' for right justification.
				flags |= FLAG_PAD_RIGHT;
				continue;
			case '0':									// A leading zero for zero pad.
				if (0 == width) {
					padchar = '0';
					continue;
				}
				// Fall through...
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8':case '9': // Read width value.
				width *= 10;
				width += c - '0';
				continue;
			case 'l': case 'L':							// A 'l' or 'L' for long integer.
				flags |= FLAG_LONG;
				continue;

#ifdef MYPRINTF_WANT_PGM_STR
			case 'S':									// String in program space.
				flags |= FLAG_STR_PGM;
#endif // MYPRINTF_WANT_PGM_STR
				// Fall through...
			case 's':									// String in normal memory.
				str.c = va_arg(ap, const char*);
				if (NULL == str.c) {					/* Catch null ptr. */
					static const char NULL_STR[] MYPRINTF_PGM_STR_DECL = "(null)";
					str.c = NULL_STR;
#ifdef MYPRINTF_WANT_PGM_STR
					flags |= FLAG_STR_PGM;				/* Set PROGMEM for AVR processor. */
#endif // MYPRINTF_WANT_PGM_STR
				}
				goto p_str;
			case 'c':									/* Single character. */
				buf[0] = (char)va_arg(ap, int);			/* Gcc can give a warning about char promotion when using va_arg(). */
				buf[1] = '\0';
				str.c = &buf[0];
				goto p_str;
			case 'd':									/* Signed decimal. */
				num.i = (flags & FLAG_LONG) ? 
				  grab_integer(CFG_MYPRINTF_T_L_INT, int) :
				  grab_integer(CFG_MYPRINTF_T_INT, int);
				if (num.i < 0) {	/* Convert to positive, note will not be sign extended as both union members are same size. */
					num.u = (CFG_MYPRINTF_T_L_UINT)-num.i;		
					flags |= FLAG_NEG;
				}
				break;
#if CFG_MYPRINTF_WANT_BINARY
			case 'b':									/* Optional binary format. */
				base = 2;
				goto do_unsigned;
#endif
			case 'X':									/* Hex (upper case). */
				flags |= FLAG_UPPER;
				// Fall through...
			case 'x':									/* Hex (lower case). */
				base = 16;
				// Fall through...
			case 'u':									/* Unsigned decimal. */
do_unsigned:	num.u = (flags & FLAG_LONG) ? 
				  grab_integer(CFG_MYPRINTF_T_L_UINT, unsigned) :
				  grab_integer(CFG_MYPRINTF_T_UINT, unsigned); 
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

			// Format digits LSD first in selected base.
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

p_str:		/* Print string `str' justified in `width' with padding char `pad'. */
			if (width > 0) { 	// Get length of string only if required.
				int len;
				const char* p = str.c;
				while ('\0' != MYPRINTF_PGM_STR_DEREF_PTR(p))
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
			while ('\0' != (c = MYPRINTF_PGM_STR_DEREF_PTR(str.c++))) {
#pragma GCC diagnostic pop
				putfunc(c, arg);
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
