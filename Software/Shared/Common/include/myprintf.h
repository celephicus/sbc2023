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


/* Generic function to write a character to whatever output device is required, arg can be used to keep track of state. */
typedef void (*myprintf_putchar)(char c, void* arg);

/* Formatter function that writes to the putfunc, passes arg directly, according to the format string fmt. */
void myprintf(myprintf_putchar putfunc, void* arg, const char* fmt, va_list ap);

/* Simple sprintf implementation. */
void myprintf_sprintf(char* buf, const char* fmt, ...);

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

void m_printf(char* str, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	myprintf(m_sprintf_of, (void*)&str, fmt, ap);
	*str = '\0';
	va_end(ap);
}
*/

#endif
