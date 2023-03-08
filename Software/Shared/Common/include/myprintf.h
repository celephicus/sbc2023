#ifndef MYPRINTF_H__
#define MYPRINTF_H__

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

/* Simple printf formatter. Formats:

	%[-0][<width>][lL]<spec>

	leading -	right justify in field width
	leading 0 	zero fill in field width (numbers only)
	<width>		field width
	l or L before spec:	number is long
	<spec>	% 	literal percent
			s	string, will print nothing for NULL pointer.
			S	string in PROGMEM (AVR target only)
			c	single character
			d	signed (possibly long) int
			u   unsigned (possibly long) int in decimal
			b	unsigned (possibly long) int in binary (optional)
			x	unsigned (possibly long) int in hex with lower case a-f
			X	unsigned (possibly long) int in hex with upper case A-F
*/


/* Signed int used for `%d' format. */
#ifndef CFG_MYPRINTF_TYPE_SIGNED_INT
#define CFG_MYPRINTF_TYPE_SIGNED_INT int16_t
#endif

/* Unsigned int used for `%x', `%x' format. */
#ifndef CFG_MYPRINTF_TYPE_UNSIGNED_INT
#define CFG_MYPRINTF_TYPE_UNSIGNED_INT uint16_t
#endif

/* Signed long int used for `%ld' format. */
#ifndef CFG_MYPRINTF_TYPE_SIGNED_LONG_INT
#define CFG_MYPRINTF_TYPE_SIGNED_LONG_INT int32_t
#endif

/* Unsigned int used for `%x', `%x' format. */
#ifndef CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT
#define CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT uint32_t
#endif

/* Function used vy myprintf to write a character to whatever output device is required, arg can be used to keep track of state. */
typedef void (*myprintf_putchar)(char c, void* arg);

/* Formatter function that writes to the putfunc, passes arg directly, according to the format string fmt. */
void myprintf(myprintf_putchar putfunc, void* arg, const char* fmt, va_list ap);

/* Safer sprintf that will not overwrite its buffer. At most (len-1) characters are written. The function returns true if all chars were were written to the buffer.
	The buffer is always terminated with a nul even on overflow. */

char myprintf_vsnprintf(char* buf, unsigned len, const char* fmt, va_list ap);
char myprintf_snprintf(char* buf, unsigned len, const char* fmt, ...);

/* Example

void m_printf_of(char c, void* arg) {
	(void)arg;
	putchar(c);
}

void m_printf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	myprintf(m_printf_of, arg, fmt, ap);
	va_end(ap);
}

void m_sprintf_of(char c, void* arg) {
	*(*(char*)arg)++ = c;
}

void m_sprintf(char* str, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	myprintf(m_sprintf_of, (void*)&str, fmt, ap);
	*str = '\0';
	va_end(ap);
}
*/

#endif
